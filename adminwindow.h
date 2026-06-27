#ifndef ADMINWINDOW_H
#define ADMINWINDOW_H

#include <QMainWindow>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QMetaType>
#include <QLabel>

namespace Ui {
class AdminWindow;
}

class AdminWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AdminWindow(
        QSqlTableModel *sharedModel,
        QSqlRelationalTableModel *sharedOrdersModel,
        QWidget *parent = nullptr
        );
    ~AdminWindow();

private slots:
    void addRow();
    void editRow();
    void updateRow();
    void clearInputs();
    void deleteRows();
    void searchByName();
    void onHeaderClicked(int logicalIndex);
    void confirmOrder();
    void rejectOrder();
    void refreshDashboard();
    void onOrderRowClicked(const QModelIndex &index);

private:
    Ui::AdminWindow *ui;
    QSqlTableModel *itemsModel;
    QSqlRelationalTableModel *ordersModel;
    QSortFilterProxyModel *proxyModel;
    QSortFilterProxyModel *ordersProxy;

    enum SortMode { None, Ascending, Descending };
    SortMode currentSortMode = None;
    int currentSortColumn = -1;
};

#endif // ADMINWINDOW_H
