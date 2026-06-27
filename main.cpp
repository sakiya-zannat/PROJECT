#include "mainwindow.h"

#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("data.db");

    if (!db.open()) {
        qDebug() << db.lastError().text();
        return -1;
    }

    QSqlQuery query;

    // Enable foreign keys (must be AFTER db.open())
    query.exec("PRAGMA foreign_keys = ON;");

    // 1. Products (parent)
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS items ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT UNIQUE NOT NULL, "
            "price REAL NOT NULL, "
            "stock INTEGER NOT NULL)"
            )) {
        qDebug() << query.lastError();
    }

    // 2. Cart (depends on items)
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS cart_items ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "user_id INTEGER NOT NULL, "
            "item_id INTEGER NOT NULL, "
            "quantity INTEGER NOT NULL, "
            "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE, "
            "FOREIGN KEY (item_id) REFERENCES items(id) ON DELETE CASCADE, "
            "UNIQUE(user_id, item_id))"
            )) {
        qDebug() << query.lastError();
    }

    // 3. Orders
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS orders ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "user_id INTEGER NOT NULL, "
            "created_at TEXT NOT NULL, "
            "status TEXT NOT NULL, "
            "FOREIGN KEY (user_id) REFERENCES users(id))"
            )) {
        qDebug() << query.lastError();
    }

    // 4. Order items (depends on orders + items)
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS order_items ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "order_id INTEGER NOT NULL, "
            "item_id INTEGER NOT NULL, "
            "quantity INTEGER NOT NULL, "
            "FOREIGN KEY (order_id) REFERENCES orders(id) "
            "ON DELETE CASCADE, "
            "FOREIGN KEY (item_id) REFERENCES items(id))"
            )) {
        qDebug() << query.lastError();
    }

    // 5. Users
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS users ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "username TEXT NOT NULL UNIQUE, "
            "password TEXT NOT NULL, "
            "role TEXT NOT NULL) "
            )) {
        qDebug() << query.lastError();
    }

    MainWindow w;
    w.show();
    return a.exec();
}
