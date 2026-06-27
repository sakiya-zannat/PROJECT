#ifndef CUSTOMERWINDOW_H
#define CUSTOMERWINDOW_H

#include <QMainWindow>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QMetaType>

namespace Ui {
class CustomerWindow;
}

class CustomerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit CustomerWindow(int usrId, QSqlTableModel *sharedModel, QSqlRelationalTableModel *sharedOrdersModel, QWidget *parent = nullptr);
    ~CustomerWindow();

private slots:
    void onHeaderClicked(int);
    void addToCart();
    void removeFromCart();
    void refreshCart();
    void handleQuantityChange(int itemId, int newQuantity);
    void updateCartTotal();
    void checkout();
    void onOrderRowClicked(const QModelIndex &index);

private:
    Ui::CustomerWindow *ui;
    QSqlTableModel *itemsModel;
    QSqlRelationalTableModel *ordersModel;
    QSortFilterProxyModel *itemsProxy;
    QSortFilterProxyModel *ordersProxy;
    QSqlQueryModel *cartModel;
    int userId;

    enum SortMode { None, Ascending, Descending };
    SortMode currentSortMode = None;
    int currentSortColumn = -1;
};

#endif // CUSTOMERWINDOW_H
