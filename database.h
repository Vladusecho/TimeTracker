#ifndef DATABASE_H
#define DATABASE_H

#include "user.h"
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QList>
#include <QString>
#include <QDateTime>

struct Task {
    int id = -1;
    QString name;
    QString project;
};

struct TimeEntry {
    int     id = -1;
    int     taskId = -1;
    QString taskName;
    QString project;
    QDateTime startTime;
    QDateTime endTime;
    int     duration = 0; // seconds
    QString description;
};

// ── Singleton для подключения к БД ──────────────────────────────────────────
class Database : public QObject
{
    Q_OBJECT
public:
    static Database& instance();

    bool init();

    // Tasks CRUD
    bool        addTask(const QString &name, const QString &project, int userId);
    bool        updateTask(int id, const QString &name, const QString &project);
    bool        deleteTask(int id);
    QList<Task> getTasks(int userId);

    // Time entries CRUD
    int              startEntry(int taskId, int userId);          // returns new entry id
    bool             stopEntry(int entryId);
    bool             updateEntry(int id, const QDateTime &start,
                                 const QDateTime &end, const QString &desc);
    bool             deleteEntry(int id);
    QList<TimeEntry> getEntries(const QDate &from, const QDate &to,
                                int taskId = -1, int userId = -1);

    // User management
    bool addUser(const QString &login, const QString &password, const QString &fullName, const QString &role);
    User getUserByLogin(const QString &login);
    User getUserById(int id);
    QList<User> getAllUsers();
    bool updateUser(int id, const QString &login, const QString &fullName, const QString &role);
    bool deleteUser(int id);
    bool changePassword(int userId, const QString &newPassword);
    bool createDefaultAdmin();
    bool migrateAddUserColumns();

private:
    explicit Database(QObject *parent = nullptr);
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    QSqlDatabase m_db;
    bool createTables();
};



#endif // DATABASE_H
