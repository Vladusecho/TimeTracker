#include "database.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include "passwordmanager.h"

Database& Database::instance()
{
    static Database inst;
    return inst;
}

Database::Database(QObject *parent) : QObject(parent) {}

bool Database::init()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dir + "/timetracker.db");

    if (!m_db.open()) {
        qWarning() << "DB open error:" << m_db.lastError().text();
        return false;
    }
    return createTables();
}

bool Database::createTables()
{
    QSqlQuery q;
    bool ok = true;

    ok &= q.exec(R"(
        CREATE TABLE IF NOT EXISTS tasks (
            id      INTEGER PRIMARY KEY AUTOINCREMENT,
            name    TEXT    NOT NULL,
            project TEXT    DEFAULT ''
        )
    )");

    ok &= q.exec(R"(
        CREATE TABLE IF NOT EXISTS time_entries (
            id          INTEGER  PRIMARY KEY AUTOINCREMENT,
            task_id     INTEGER  NOT NULL REFERENCES tasks(id) ON DELETE CASCADE,
            start_time  TEXT     NOT NULL,
            end_time    TEXT,
            duration    INTEGER  DEFAULT 0,
            description TEXT     DEFAULT ''
        )
    )");

    ok &= q.exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            login       TEXT    NOT NULL UNIQUE,
            password    TEXT    NOT NULL,
            full_name   TEXT    NOT NULL,
            role        TEXT    NOT NULL DEFAULT 'worker'
        )
    )");
    migrateAddUserColumns();
    createDefaultAdmin();
    if (!ok) qWarning() << "createTables error:" << q.lastError().text();
    return ok;
}

bool Database::migrateAddUserColumns()
{
    QSqlQuery q;

    // Проверяем, есть ли колонка user_id в tasks
    q.exec("PRAGMA table_info(tasks)");
    bool hasUserId = false;
    while (q.next()) {
        if (q.value(1).toString() == "user_id") {
            hasUserId = true;
            break;
        }
    }

    if (!hasUserId) {
        q.exec("ALTER TABLE tasks ADD COLUMN user_id INTEGER REFERENCES users(id) DEFAULT 1");
    }

    // Проверяем колонку user_id в time_entries
    q.exec("PRAGMA table_info(time_entries)");
    hasUserId = false;
    while (q.next()) {
        if (q.value(1).toString() == "user_id") {
            hasUserId = true;
            break;
        }
    }

    if (!hasUserId) {
        q.exec("ALTER TABLE time_entries ADD COLUMN user_id INTEGER REFERENCES users(id) DEFAULT 1");
    }

    return true;
}

// ── Tasks ────────────────────────────────────────────────────────────────────

bool Database::addTask(const QString &name, const QString &project, int userId)
{
    QSqlQuery q;
    q.prepare("INSERT INTO tasks (name, project, user_id) VALUES (?, ?, ?)");
    q.addBindValue(name);
    q.addBindValue(project);
    q.addBindValue(userId);
    return q.exec();
}

bool Database::updateTask(int id, const QString &name, const QString &project)
{
    QSqlQuery q;
    q.prepare("UPDATE tasks SET name=?, project=? WHERE id=?");
    q.addBindValue(name);
    q.addBindValue(project);
    q.addBindValue(id);
    return q.exec();
}

bool Database::deleteTask(int id)
{
    QSqlQuery q;
    q.prepare("DELETE FROM tasks WHERE id=?");
    q.addBindValue(id);
    return q.exec();
}

QList<Task> Database::getTasks(int userId)
{
    QList<Task> list;
    QSqlQuery q;

    if (userId > 0) {
        // Для конкретного пользователя
        q.prepare("SELECT id, name, project FROM tasks WHERE user_id = ? OR user_id IS NULL ORDER BY name");
        q.addBindValue(userId);
    } else if (userId == -1) {
        // Для администратора - все задачи
        q.prepare("SELECT id, name, project FROM tasks ORDER BY name");
    } else {
        return list;
    }

    q.exec();
    while (q.next()) {
        Task t;
        t.id      = q.value(0).toInt();
        t.name    = q.value(1).toString();
        t.project = q.value(2).toString();
        list.append(t);
    }
    return list;
}


// ── Time Entries ─────────────────────────────────────────────────────────────

int Database::startEntry(int taskId, int userId)
{
    QSqlQuery q;
    q.prepare("INSERT INTO time_entries (task_id, start_time, user_id) VALUES (?, ?, ?)");
    q.addBindValue(taskId);
    q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    q.addBindValue(userId);
    if (!q.exec()) return -1;
    return q.lastInsertId().toInt();
}

bool Database::stopEntry(int entryId)
{
    QDateTime end = QDateTime::currentDateTime();
    // get start to calc duration
    QSqlQuery q;
    q.prepare("SELECT start_time FROM time_entries WHERE id=?");
    q.addBindValue(entryId);
    q.exec(); q.next();
    QDateTime start = QDateTime::fromString(q.value(0).toString(), Qt::ISODate);
    int secs = static_cast<int>(start.secsTo(end));

    q.prepare("UPDATE time_entries SET end_time=?, duration=? WHERE id=?");
    q.addBindValue(end.toString(Qt::ISODate));
    q.addBindValue(secs);
    q.addBindValue(entryId);
    return q.exec();
}

bool Database::updateEntry(int id, const QDateTime &start,
                           const QDateTime &end, const QString &desc)
{
    int secs = static_cast<int>(start.secsTo(end));
    QSqlQuery q;
    q.prepare("UPDATE time_entries SET start_time=?, end_time=?, duration=?, description=? WHERE id=?");
    q.addBindValue(start.toString(Qt::ISODate));
    q.addBindValue(end.toString(Qt::ISODate));
    q.addBindValue(secs);
    q.addBindValue(desc);
    q.addBindValue(id);
    return q.exec();
}

bool Database::deleteEntry(int id)
{
    QSqlQuery q;
    q.prepare("DELETE FROM time_entries WHERE id=?");
    q.addBindValue(id);
    return q.exec();
}

QList<TimeEntry> Database::getEntries(const QDate &from, const QDate &to, int taskId, int userId)
{
    QList<TimeEntry> list;
    QString sql = R"(
        SELECT e.id, e.task_id, t.name, t.project,
               e.start_time, e.end_time, e.duration, e.description
        FROM time_entries e
        JOIN tasks t ON t.id = e.task_id
        WHERE date(e.start_time) >= ? AND date(e.start_time) <= ?
    )";

    if (taskId > 0) sql += " AND e.task_id = ?";
    if (userId > 0) sql += " AND e.user_id = ?";
    sql += " ORDER BY e.start_time DESC";

    QSqlQuery q;
    q.prepare(sql);
    q.addBindValue(from.toString(Qt::ISODate));
    q.addBindValue(to.toString(Qt::ISODate));
    if (taskId > 0) q.addBindValue(taskId);
    if (userId > 0) q.addBindValue(userId);
    q.exec();

    while (q.next()) {
        TimeEntry e;
        e.id          = q.value(0).toInt();
        e.taskId      = q.value(1).toInt();
        e.taskName    = q.value(2).toString();
        e.project     = q.value(3).toString();
        e.startTime   = QDateTime::fromString(q.value(4).toString(), Qt::ISODate);
        e.endTime     = q.value(5).isNull() ? QDateTime() :
                        QDateTime::fromString(q.value(5).toString(), Qt::ISODate);
        e.duration    = q.value(6).toInt();
        e.description = q.value(7).toString();
        list.append(e);
    }
    return list;
}

bool Database::addUser(const QString &login, const QString &password,
                       const QString &fullName, const QString &role)
{
    QSqlQuery q;
    q.prepare("INSERT INTO users (login, password, full_name, role) VALUES (?, ?, ?, ?)");
    q.addBindValue(login);
    q.addBindValue(PasswordManager::hashPassword(password));
    q.addBindValue(fullName);
    q.addBindValue(role);
    return q.exec();
}

User Database::getUserByLogin(const QString &login)
{
    User user;
    QSqlQuery q;
    q.prepare("SELECT id, login, password, full_name, role FROM users WHERE login = ?");
    q.addBindValue(login);

    if (q.exec() && q.next()) {
        user.id = q.value(0).toInt();
        user.login = q.value(1).toString();
        user.passwordHash = q.value(2).toString();
        user.fullName = q.value(3).toString();
        user.role = q.value(4).toString();
    }
    return user;
}

User Database::getUserById(int id)
{
    User user;
    QSqlQuery q;
    q.prepare("SELECT id, login, password, full_name, role FROM users WHERE id = ?");
    q.addBindValue(id);

    if (q.exec() && q.next()) {
        user.id = q.value(0).toInt();
        user.login = q.value(1).toString();
        user.passwordHash = q.value(2).toString();
        user.fullName = q.value(3).toString();
        user.role = q.value(4).toString();
    }
    return user;
}

QList<User> Database::getAllUsers()
{
    QList<User> users;
    QSqlQuery q("SELECT id, login, password, full_name, role FROM users ORDER BY login");

    while (q.next()) {
        User user;
        user.id = q.value(0).toInt();
        user.login = q.value(1).toString();
        user.passwordHash = q.value(2).toString();
        user.fullName = q.value(3).toString();
        user.role = q.value(4).toString();
        users.append(user);
    }
    return users;
}

bool Database::updateUser(int id, const QString &login, const QString &fullName, const QString &role)
{
    QSqlQuery q;
    q.prepare("UPDATE users SET login=?, full_name=?, role=? WHERE id=?");
    q.addBindValue(login);
    q.addBindValue(fullName);
    q.addBindValue(role);
    q.addBindValue(id);
    return q.exec();
}

bool Database::deleteUser(int id)
{
    // Нельзя удалить администратора, если он последний
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM users WHERE role='admin'");
    q.exec();
    q.next();
    int adminCount = q.value(0).toInt();

    User user = getUserById(id);
    if (user.role == "admin" && adminCount <= 1) {
        qWarning() << "Cannot delete the last admin user";
        return false;
    }

    q.prepare("DELETE FROM users WHERE id=?");
    q.addBindValue(id);
    return q.exec();
}

bool Database::changePassword(int userId, const QString &newPassword)
{
    QSqlQuery q;
    q.prepare("UPDATE users SET password=? WHERE id=?");
    q.addBindValue(PasswordManager::hashPassword(newPassword));
    q.addBindValue(userId);
    return q.exec();
}

bool Database::createDefaultAdmin()
{
    // Проверяем, есть ли хоть один пользователь
    QSqlQuery check("SELECT COUNT(*) FROM users");
    check.exec();
    check.next();

    if (check.value(0).toInt() == 0) {
        // Создаем администратора по умолчанию
        return addUser("admin", "admin123", "Системный администратор", "admin");
    }
    return true;
}
