#ifndef USER_H
#define USER_H

#include <QString>

struct User
{
    int id = -1;
    QString login;
    QString passwordHash;
    QString fullName;
    QString role; // "admin" или "worker"

    bool isAdmin() const { return role == "admin"; }
    bool isWorker() const { return role == "worker"; }
};

#endif // USER_H
