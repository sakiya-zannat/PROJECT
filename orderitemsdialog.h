#ifndef ORDERITEMSDIALOG_H
#define ORDERITEMSDIALOG_H
#include <QSqlQueryModel>

#include <QDialog>

namespace Ui {
class OrderItemsDialog;
}

class OrderItemsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OrderItemsDialog(int id, QWidget *parent = nullptr);
    ~OrderItemsDialog();

private:
    Ui::OrderItemsDialog *ui;
    QSqlQueryModel* orderItemsModel;
    int orderId;
};

#endif // ORDERITEMSDIALOG_H
