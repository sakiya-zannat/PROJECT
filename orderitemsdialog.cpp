#include "orderitemsdialog.h"
#include "mainwindow.h"
#include "ui_orderitemsdialog.h"
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlError>

OrderItemsDialog::OrderItemsDialog(int id, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OrderItemsDialog)
    , orderId(id)
{
    ui->setupUi(this);
    setWindowTitle("Order Information");
    orderItemsModel = new QSqlQueryModel(this);

    QSqlQuery q;
    q.prepare(
        "SELECT "
        "items.name AS name, "
        "items.price AS price, "
        "order_items.quantity AS quantity, "
        "(items.price * order_items.quantity) AS total "
        "FROM order_items "
        "JOIN items ON items.id = order_items.item_id "
        "WHERE order_items.order_id = ?"
        );
    q.addBindValue(orderId);
    q.exec();
    orderItemsModel->setQuery(std::move(q));

    ui->tableView->setModel(orderItemsModel);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setFocusPolicy(Qt::NoFocus);
    ui->tableView->setStyleSheet(R"(
        QTableView::item:selected {
            background-color: #d0eaff;
            color: black;
        }
        )");

    auto header = ui->tableView->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    NumericDelegate *numericDelegate = new NumericDelegate(ui->tableView);
    ui->tableView->setItemDelegateForColumn(1, numericDelegate);
    ui->tableView->setItemDelegateForColumn(2, numericDelegate);
    ui->tableView->setItemDelegateForColumn(3, numericDelegate);

    QSqlQuery subtotalQuery;
    subtotalQuery.prepare(
        "SELECT SUM(i.price * oi.quantity) "
        "FROM order_items oi "
        "JOIN items i ON oi.item_id = i.id "
        "WHERE order_id = ?"
        );
    subtotalQuery.addBindValue(orderId);
    subtotalQuery.exec();

    double subtotal = 0.0;
    if (subtotalQuery.exec()) {
        if (subtotalQuery.next()) {
            subtotal = subtotalQuery.value(0).toDouble();
        }
    } else {
        qDebug() << "Database Error:" << subtotalQuery.lastError().text();
    }

    ui->subtotal_label->setText(
        QString("Subtotal: BDT %1").arg(subtotal, 0, 'f', 2)
        );
    ui->subtotal_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->subtotal_label->setStyleSheet(R"(
        QLabel {
            font-weight: bold;
            font-size: 14px;
            background-color: #f9f9f9;
            border-top: 1px solid #d0eaff;
            padding: 4px 8px;
        }
        )");

}

OrderItemsDialog::~OrderItemsDialog()
{
    delete ui;
}
