#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "adminwindow.h"
#include "customerwindow.h"
#include <QSqlRelationalTableModel>
#include <QMainWindow>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QSqlTableModel *sharedModel;
    QSqlRelationalTableModel *sharedOrdersModel;

private slots:
    void openAdminWindow();
    void openCustomerWindow(int);
    void registerUser();
    void login();

private:
    Ui::MainWindow *ui;
    AdminWindow *adminWindow;
    QMap<int, CustomerWindow*> customerWindows;
};

class NumericDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);

        if (!index.isValid())
            return;

        QVariant value = index.data();

        if (value.metaType().id() == QMetaType::Double) {
            option->text = QString::number(value.toDouble(), 'f', 2); // 2 decimals
        } else if (value.metaType().id() == QMetaType::Int) {
            option->text = QString::number(value.toInt());
        }

        option->displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
    }
};

#endif // MAINWINDOW_H
