#ifndef PASSWORDMANAGER_H
#define PASSWORDMANAGER_H

#include <QString>
#include <QCryptographicHash>

class PasswordManager
{
public:
    static QString hashPassword(const QString &password)
    {
        QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
        return hash.toHex();
    }

    static bool verifyPassword(const QString &password, const QString &hash)
    {
        return hashPassword(password) == hash;
    }
};

#endif // PASSWORDMANAGER_H
