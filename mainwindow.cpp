#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "adminwindow.h"
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlRelationalTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QMap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , adminWindow(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("E-commerce Management System");

    sharedModel = new QSqlTableModel(this);
    sharedModel->setTable("items");
    sharedModel->select();
    sharedModel->setEditStrategy(QSqlTableModel::OnManualSubmit);

    sharedOrdersModel = new QSqlRelationalTableModel(this);
    sharedOrdersModel->setTable("orders");
    sharedOrdersModel->setEditStrategy(QSqlRelationalTableModel::OnManualSubmit);

    sharedOrdersModel->setRelation(
        sharedOrdersModel->fieldIndex("user_id"),
        QSqlRelation("users", "id", "username")
        );

    sharedOrdersModel->setHeaderData(
        sharedOrdersModel->fieldIndex("user_id"),
        Qt::Horizontal,
        "user"
        );

    sharedOrdersModel->select();

    connect(ui->register_btn, &QPushButton::clicked,
            this, &MainWindow::registerUser);
    connect(ui->login_btn, &QPushButton::clicked,
            this, &MainWindow::login);
}

void MainWindow::login()
{
    QString username = ui->user_edit->text().trimmed();
    QString password = username;

    if (username.isEmpty()) {
        QMessageBox::warning(this, "Login", "Fill all fields");
        return;
    }

    QSqlQuery query;
    query.prepare(
        "SELECT id, role FROM users "
        "WHERE username = ? AND password = ?"
        );
    query.addBindValue(username);
    query.addBindValue(password);

    if (!query.exec()) {
        QMessageBox::critical(this, "Error", query.lastError().text());
        return;
    }

    if (!query.next()) {
        QMessageBox::warning(this, "Login", "Invalid credentials");
        return;
    }

    int userId = query.value("id").toInt();
    QString role = query.value("role").toString();

    if (role == "admin") {
        openAdminWindow();
    } else {
        openCustomerWindow(userId);
    }
}

void MainWindow::registerUser()
{
    QString username = ui->newUser_edit->text().trimmed();
    QString password = username;

    if (username.isEmpty()) {
        QMessageBox::warning(this, "Registration", "Please fill in all fields");
        return;
    }

    QSqlQuery query;

    // Check if username already exists
    query.prepare("SELECT id FROM users WHERE username = ?");
    query.addBindValue(username);

    if (!query.exec()) {
        QMessageBox::critical(this, "Database Error", query.lastError().text());
        return;
    }

    if (query.next()) {
        QMessageBox::warning(this, "Registration", "Username already exists");
        return;
    }

    // Insert new customer
    query.prepare(
        "INSERT INTO users (username, password, role) "
        "VALUES (?, ?, ?)"
        );
    query.addBindValue(username);
    query.addBindValue(password);
    query.addBindValue("customer");

    if (!query.exec()) {
        QMessageBox::critical(this, "Registration Failed", query.lastError().text());
        return;
    }

    QMessageBox::information(this, "Registration", "Account created successfully");

    // Clear fields
    ui->newUser_edit->clear();
    ui->newPass_edit->clear();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openAdminWindow()
{
    if (!adminWindow) {
        adminWindow = new AdminWindow(sharedModel, sharedOrdersModel, this);
        adminWindow->setAttribute(Qt::WA_DeleteOnClose);

        connect(adminWindow, &QObject::destroyed, this, [this]() {
            adminWindow = nullptr;
        });
        adminWindow->show();
    } else {
        adminWindow->raise();
        adminWindow->activateWindow();
    }
    // Clear fields
    ui->user_edit->clear();
    ui->pass_edit->clear();
}

void MainWindow::openCustomerWindow(int userId)
{
    // If this user already has an open window
    if (customerWindows.contains(userId)) {
        CustomerWindow* win = customerWindows[userId];
        win->raise();
        win->activateWindow();
        return;
    }

    // Create new customer window for this user
    CustomerWindow* win = new CustomerWindow(
        userId, sharedModel, sharedOrdersModel, this);

    win->setAttribute(Qt::WA_DeleteOnClose);

    // Remove from map when window is closed
    connect(win, &QObject::destroyed, this, [this, userId]() {
        customerWindows.remove(userId);
    });

    customerWindows.insert(userId, win);
    win->show();

    // Clear fields
    ui->user_edit->clear();
    ui->pass_edit->clear();
}
