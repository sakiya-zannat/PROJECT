#include "adminwindow.h"
#include "mainwindow.h"
#include "orderitemsdialog.h"
#include "ui_adminwindow.h"
#include <QMessageBox>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QSqlQuery>
#include <QSqlError>
#include <QFont>

AdminWindow::AdminWindow(QSqlTableModel *sharedModel, QSqlRelationalTableModel *sharedOrdersModel, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::AdminWindow)
    , itemsModel(sharedModel)
    , ordersModel(sharedOrdersModel)
{
    ui->setupUi(this);
    setWindowTitle("Admin Panel");

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(itemsModel);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterKeyColumn(itemsModel->fieldIndex("name"));

    ui->items_table->setModel(proxyModel);
    ui->items_table->hideColumn(itemsModel->fieldIndex("id"));
    ui->items_table->horizontalHeader()->setMinimumSectionSize(100);
    ui->items_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->items_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->items_table->setFocusPolicy(Qt::NoFocus);
    ui->items_table->setStyleSheet(R"(
        QTableView::item:selected {
            background-color: #d0eaff;
            color: black;
        }
        )");

    int nameCol = itemsModel->fieldIndex("name");
    int priceCol = itemsModel->fieldIndex("price");
    int qtyCol   = itemsModel->fieldIndex("stock");

    auto header = ui->items_table->horizontalHeader();
    header->setSectionResizeMode(nameCol, QHeaderView::Stretch);
    header->setSectionResizeMode(priceCol, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(qtyCol, QHeaderView::ResizeToContents);

    NumericDelegate *numericDelegate = new NumericDelegate(ui->items_table);

    ui->items_table->setItemDelegateForColumn(priceCol, numericDelegate);
    ui->items_table->setItemDelegateForColumn(qtyCol, numericDelegate);

    connect(ui->items_table->horizontalHeader(), &QHeaderView::sectionClicked,
            this, &AdminWindow::onHeaderClicked);
    connect(ui->add_btn, &QPushButton::clicked, this, &AdminWindow::addRow);
    connect(ui->edit_btn, &QPushButton::clicked, this, &AdminWindow::editRow);
    connect(ui->up_btn, &QPushButton::clicked, this, &AdminWindow::updateRow);
    connect(ui->clear_btn, &QPushButton::clicked, this, &::AdminWindow::clearInputs);
    connect(ui->del_btn, &QPushButton::clicked, this, &::AdminWindow::deleteRows);
    connect(ui->search_btn, &QPushButton::clicked,this, &AdminWindow::searchByName);

    // For price: allow decimals (e.g., 0.00 to 999999.99, 2 decimal places)
    QDoubleValidator *priceValidator = new QDoubleValidator(0, 999999, 2, this);
    priceValidator->setNotation(QDoubleValidator::StandardNotation);
    ui->price_edit->setValidator(priceValidator);

    // For quantity: integers only (e.g., 0 to 999999)
    QIntValidator *quantityValidator = new QIntValidator(0, 999999, this);
    ui->quantity_edit->setValidator(quantityValidator);

    ordersProxy = new QSortFilterProxyModel(this);
    ordersProxy->setSourceModel(ordersModel);
    ordersProxy->setDynamicSortFilter(true);
    ordersProxy->sort(ordersModel->fieldIndex("id"), Qt::DescendingOrder);

    ui->orders_table->setModel(ordersProxy);
    ui->orders_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->orders_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->orders_table->setFocusPolicy(Qt::NoFocus);
    ui->orders_table->setStyleSheet(R"(
        QTableView::item:selected {
            background-color: #d0eaff;
            color: black;
        }
        )");
    connect(ui->order_confirm_btn, &QPushButton::clicked,this, &AdminWindow::confirmOrder);
    connect(ui->order_reject_btn, &QPushButton::clicked,this, &AdminWindow::rejectOrder);

    int idCol = ordersModel->fieldIndex("id");
    int userCol = ordersModel->fieldIndex("username");
    int timeCol = ordersModel->fieldIndex("created_at");
    int statusCol   = ordersModel->fieldIndex("status");

    header = ui->orders_table->horizontalHeader();
    header->setSectionResizeMode(idCol, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(userCol, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(timeCol, QHeaderView::Stretch);
    header->setSectionResizeMode(statusCol, QHeaderView::ResizeToContents);

    refreshDashboard();
    connect(itemsModel, &QSqlTableModel::modelReset, this, &AdminWindow::refreshDashboard);
    connect(ordersModel, &QSqlRelationalTableModel::modelReset, this, &AdminWindow::refreshDashboard);

    ui->orders_table->setToolTip("Double-click an order to view details");
    connect(ui->orders_table,&QTableView::doubleClicked, this, &AdminWindow::onOrderRowClicked);
}

void AdminWindow::onOrderRowClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    int idCol = ordersModel->fieldIndex("id");
    QModelIndex sourceIndex = ordersProxy->mapToSource(index);
    int orderId = sourceIndex
                      .siblingAtColumn(idCol)
                      .data()
                      .toInt();

    OrderItemsDialog dlg(orderId, this);
    dlg.exec();
}

void AdminWindow::refreshDashboard()
{
    QSqlQuery query;

    QFont boldFont;
    boldFont.setBold(true);
    boldFont.setPixelSize(14);

    // 1. Total items in stock (sum of quantities)
    int totalStock = 0;
    if (query.exec("SELECT SUM(stock) FROM items") && query.next()) {
        totalStock = query.value(0).toInt();
    } else {
        qDebug() << "Error calculating total items:" << query.lastError();
    }
    ui->tot_stock_label->setText(QString("Total Stock: %1").arg(totalStock));
    ui->tot_stock_label->setAlignment(Qt::AlignCenter);
    ui->tot_stock_label->setFont(boldFont);
    ui->tot_stock_label->setStyleSheet("color: #2E86C1;"); // Blue-ish

    // 2. Total orders
    int totalOrders = 0;
    if (query.exec("SELECT COUNT(*) FROM orders") && query.next()) {
        totalOrders = query.value(0).toInt();
    } else {
        qDebug() << "Error counting orders:" << query.lastError();
    }
    ui->tot_orders_label->setText(QString("Total Orders: %1").arg(totalOrders));
    ui->tot_orders_label->setAlignment(Qt::AlignCenter);
    ui->tot_orders_label->setFont(boldFont);
    ui->tot_orders_label->setStyleSheet("color: #27AE60;"); // Green

    // 3. Total sales (sum of quantity * price from confirmed orders)
    double totalSales = 0.0;
    if (query.exec(
            "SELECT SUM(oi.quantity * i.price) "
            "FROM order_items oi "
            "JOIN items i ON oi.item_id = i.id "
            "JOIN orders o ON oi.order_id = o.id "
            "WHERE o.status = 'Confirmed'"
            ) && query.next()) {
        totalSales = query.value(0).toDouble();
    } else {
        qDebug() << "Error calculating total sales:" << query.lastError();
    }

    // Format with 2 decimals
    ui->tot_sales_label->setText(QString("Sales: BDT %1").arg(QString::number(totalSales, 'f', 2)));
    ui->tot_sales_label->setAlignment(Qt::AlignCenter);
    ui->tot_sales_label->setFont(boldFont);
    ui->tot_sales_label->setStyleSheet("color: #C0392B;"); // Red-ish
}

void AdminWindow::rejectOrder() {
    QModelIndexList selectedRows = ui->orders_table->selectionModel()->selectedRows();
        if (selectedRows.size() != 1) {
        QMessageBox::warning(this, "Reject Order", "Please select exactly one order to reject.");
        return;
    }
    QModelIndex proxyIndex = ui->orders_table->currentIndex();
    if (!proxyIndex.isValid())
        return;

    QModelIndex sourceIndex = ordersProxy->mapToSource(selectedRows.first());
    int statusCol = ordersModel->fieldIndex("status");
    QModelIndex statusIndex = sourceIndex.siblingAtColumn(statusCol);
    QString currentStatus = statusIndex.data().toString();

    // 🚫 Already rejected → block
    if (currentStatus == "Rejected") {
        QMessageBox::information(this, "Reject Order",
                                 "This order is already rejected.");
        return;
    }

    // ⚠ Already confirmed → ask
    if (currentStatus == "Confirmed") {
        auto reply = QMessageBox::question(
            this,
            "Reject Confirmed Order",
            "This order is already confirmed.\n"
            "Are you sure you want to change its status to Rejected?",
            QMessageBox::Yes | QMessageBox::No
            );

        if (reply != QMessageBox::Yes)
            return;
    }

    if (currentStatus == "Confirmed") {
        // Fetch all order items        
        int idCol = ordersModel->fieldIndex("id");
        int orderId = sourceIndex.siblingAtColumn(idCol).data().toInt();
        QSqlQuery q;
        q.prepare("SELECT item_id, quantity FROM order_items WHERE order_id = ?");
        q.addBindValue(orderId);
        if (!q.exec()) {
            QMessageBox::critical(this, "Error", q.lastError().text());
            return;
        }

        QSqlDatabase db = QSqlDatabase::database();
        db.transaction();
        QSqlQuery updateStock;
        updateStock.prepare("UPDATE items SET stock = stock + ? WHERE id = ?");

        while (q.next()) {
            int itemId = q.value("item_id").toInt();
            int qty = q.value("quantity").toInt();
            updateStock.addBindValue(qty);
            updateStock.addBindValue(itemId);
            if (!updateStock.exec()) {
                db.rollback();
                QMessageBox::critical(this, "Error", updateStock.lastError().text());
                return;
            }
        }
        db.commit();
    }

    // ✅ Update status
    ordersModel->setData(statusIndex, "Rejected");

    if (!ordersModel->submitAll()) {
        QMessageBox::warning(this, "Update failed", ordersModel->lastError().text());
        ordersModel->revertAll();
        return;
    }
    itemsModel->select();
    ordersModel->select();
}

void AdminWindow::confirmOrder()
{
    QModelIndexList selectedRows =
        ui->orders_table->selectionModel()->selectedRows();

    if (selectedRows.size() != 1) {
        QMessageBox::warning(this, "Confirm Order",
                             "Please select exactly one order to confirm.");
        return;
    }

    QModelIndex proxyIndex = selectedRows.first();

    // 2. Map it to the actual source model index
    QModelIndex sourceIndex = ordersProxy->mapToSource(proxyIndex);

    // 3. Define your status index correctly using the mapped row
    int statusCol = ordersModel->fieldIndex("status");
    QModelIndex statusIndex = sourceIndex.siblingAtColumn(statusCol);

    QString currentStatus = statusIndex.data().toString();

    // 🚫 Already confirmed → block
    if (currentStatus == "Confirmed") {
        QMessageBox::information(this, "Confirm Order",
                                 "This order is already confirmed.");
        return;
    }

    // ⚠ Rejected → ask before overriding
    if (currentStatus == "Rejected") {
        auto reply = QMessageBox::question(
            this,
            "Confirm Rejected Order",
            "This order is currently rejected.\n"
            "Are you sure you want to change its status to Confirmed?",
            QMessageBox::Yes | QMessageBox::No
            );

        if (reply != QMessageBox::Yes)
            return;
    }

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) {
        QMessageBox::critical(this, "Error", "Failed to start transaction.");
        return;
    }

    QSqlQuery q;
    QStringList problems;

    int idCol = ordersModel->fieldIndex("id");
    int orderId = sourceIndex.siblingAtColumn(idCol).data().toInt();

    q.prepare(
        "SELECT oi.item_id, oi.quantity, i.stock, i.name "
        "FROM order_items oi "
        "JOIN items i ON i.id = oi.item_id "
        "WHERE oi.order_id = ?"
        );
    q.addBindValue(orderId);

    if (!q.exec()) {
        db.rollback();
        QMessageBox::critical(this, "Error", q.lastError().text());
        return;
    }

    struct Item {
        int id;
        int qty;
    };

    QVector<Item> items;

    while (q.next()) {
        int stock = q.value("stock").toInt();
        int qty   = q.value("quantity").toInt();

        if (stock < qty) {
            problems << QString("%1 (needed %2, available %3)")
            .arg(q.value("name").toString())
                .arg(qty).arg(stock);
        }

        items.push_back({ q.value("item_id").toInt(), qty });
    }

    if (!problems.isEmpty()) {
        db.rollback();
        QMessageBox::warning(this, "Insufficient Stock",
                             problems.join("\n"));
        return;
    }

    // Update stock
    QSqlQuery updateStock;
    updateStock.prepare(
        "UPDATE items SET stock = stock - ? WHERE id = ?"
        );

    for (const auto &item : items) {
        updateStock.addBindValue(item.qty);
        updateStock.addBindValue(item.id);

        if (!updateStock.exec()) {
            db.rollback();
            QMessageBox::critical(this, "Error",
                                  updateStock.lastError().text());
            return;
        }
    }

    // Update order status
    ordersModel->setData(statusIndex, "Confirmed");
    if (!ordersModel->submitAll()) {
        db.rollback();
        ordersModel->revertAll();
        QMessageBox::warning(this, "Update failed",
                             ordersModel->lastError().text());
        return;
    }

    db.commit();
    itemsModel->select();
    ordersModel->select();

    QMessageBox::information(this, "Order Confirmed",
                             "Order confirmed and stock updated.");
}

AdminWindow::~AdminWindow()
{
    delete ui;
}

void AdminWindow::addRow()
{
    QString name = ui->name_edit->text().trimmed();
    QString price = ui->price_edit->text().trimmed();
    QString quantity = ui->quantity_edit->text().trimmed();

    if (name.isEmpty() || price.isEmpty() || quantity.isEmpty()) {
        QMessageBox::warning(this, "Add Item", "Please fill in all fields.");
        return;
    }

    int row = itemsModel->rowCount();
    itemsModel->insertRow(row);

    itemsModel->setData(itemsModel->index(row, itemsModel->fieldIndex("name")), name);
    itemsModel->setData(itemsModel->index(row, itemsModel->fieldIndex("price")), price);
    itemsModel->setData(itemsModel->index(row, itemsModel->fieldIndex("stock")), quantity);

    if (!itemsModel->submitAll()) {
        QMessageBox::warning(this, "DB Error", itemsModel->lastError().text());
        itemsModel->revertAll();
        return;
    }

    clearInputs();
}

void AdminWindow::editRow()
{
    QModelIndexList selectedRows = ui->items_table->selectionModel()->selectedRows();

    if (selectedRows.size() != 1) {
        QMessageBox::warning(this, "Edit Row", "Please select exactly one row to edit.");
        return;
    }

    QModelIndex proxyIndex = ui->items_table->currentIndex();
    if (!proxyIndex.isValid())
        return;

    QModelIndex src = proxyModel->mapToSource(proxyIndex);
    int row = src.row();

    ui->name_edit->setText(
        itemsModel->data(itemsModel->index(row, itemsModel->fieldIndex("name"))).toString()
        );
    ui->price_edit->setText(
        QString::number(itemsModel->data(itemsModel->index(row, itemsModel->fieldIndex("price"))).toDouble(), 'f', 2)
        );
    ui->quantity_edit->setText(
        QString::number(itemsModel->data(itemsModel->index(row, itemsModel->fieldIndex("stock"))).toInt())
        );
}

void AdminWindow::updateRow()
{
    QModelIndexList selectedRows = ui->items_table->selectionModel()->selectedRows();
    if (selectedRows.size() != 1) {
        QMessageBox::warning(this, "Update Row", "Please select exactly one row to update.");
        return;
    }

    QString name = ui->name_edit->text().trimmed();
    QString price = ui->price_edit->text().trimmed();
    QString quantity = ui->quantity_edit->text().trimmed();

    if (name.isEmpty() || price.isEmpty() || quantity.isEmpty()) {
        QMessageBox::warning(this, "Update Row", "Please fill in all fields.");
        return;
    }

    QModelIndex proxyIndex = ui->items_table->currentIndex();
    if (!proxyIndex.isValid())
        return;

    QModelIndex src = proxyModel->mapToSource(proxyIndex);
    int row = src.row();

    itemsModel->setData(itemsModel->index(row, itemsModel->fieldIndex("name")),
                        ui->name_edit->text().trimmed());
    itemsModel->setData(itemsModel->index(row, itemsModel->fieldIndex("price")),
                        ui->price_edit->text().toDouble());
    itemsModel->setData(itemsModel->index(row, itemsModel->fieldIndex("stock")),
                        ui->quantity_edit->text().toInt());

    if (!itemsModel->submitAll()) {
        QMessageBox::warning(this, "Update failed", itemsModel->lastError().text());
        itemsModel->revertAll();
        return;
    }

    // Clear input fields
    ui->name_edit->clear();
    ui->price_edit->clear();
    ui->quantity_edit->clear();
}

void AdminWindow::clearInputs()
{
    // Clear input fields
    ui->name_edit->clear();
    ui->price_edit->clear();
    ui->quantity_edit->clear();
    ui->items_table->clearSelection();
    statusBar()->clearMessage();
    ui->clear_btn->setStyleSheet("");
    proxyModel->setFilterFixedString("");
    itemsModel->select();

}

void AdminWindow::deleteRows()
{
    QModelIndexList proxyRows = ui->items_table->selectionModel()->selectedRows();

    if (proxyRows.isEmpty()) {
        QMessageBox::information(this, "Delete Row", "Please select at least one row to delete.");
        return;
    }

    int ret = QMessageBox::question(
        this,
        "Delete Items",
        QString("Delete %1 selected item(s)?").arg(proxyRows.size()),
        QMessageBox::Yes | QMessageBox::No
        );

    if (ret != QMessageBox::Yes)
        return;

    // Convert proxy rows → source rows
    QList<int> sourceRows;
    for (const QModelIndex &proxyIndex : std::as_const(proxyRows)) {
        sourceRows.append(proxyModel->mapToSource(proxyIndex).row());
    }

    // Sort rows descending (CRITICAL)
    std::sort(sourceRows.begin(), sourceRows.end(), std::greater<int>());

    // Remove rows
    for (int row : sourceRows) {
        itemsModel->removeRow(row);
    }

    // Commit changes
    if (!itemsModel->submitAll()) {
        QMessageBox::warning(this, "Delete failed",
                             itemsModel->lastError().text());
        itemsModel->revertAll();
        return;
    }

    clearInputs();
}

void AdminWindow::searchByName()
{
    QString keyword = ui->name_edit->text().trimmed();
    proxyModel->setFilterFixedString(keyword);
    if (keyword.isEmpty()) {
        statusBar()->clearMessage();
        ui->clear_btn->setStyleSheet("");
        QMessageBox::information(this, "Search", "Please enter a name to search.");
    } else {
        const QString searchActiveStyle = R"(
            QPushButton {
                border: 1px solid #cc4444; color: #880000; font-weight: bold;
            }
            QPushButton:hover {
                background-color: #ffcccc;
            }
        )";
        ui->clear_btn->setStyleSheet(searchActiveStyle);
        statusBar()->showMessage("Showing search results — click Clear to see all items");
    }
}

void AdminWindow::onHeaderClicked(int logicalIndex)
{
    if (currentSortColumn != logicalIndex) {
        // New column clicked → start with ascending
        currentSortColumn = logicalIndex;
        currentSortMode = Ascending;
    } else {
        // Cycle sort mode
        switch (currentSortMode) {
        case None: currentSortMode = Ascending; break;
        case Ascending: currentSortMode = Descending; break;
        case Descending: currentSortMode = None; break;
        }
    }

    // Apply sorting
    if (currentSortMode == None) {
        proxyModel->sort(-1); // reset sorting
        ui->items_table->horizontalHeader()->setSortIndicatorShown(false);
    } else if (currentSortMode == Ascending) {
        proxyModel->sort(logicalIndex, Qt::AscendingOrder);
        ui->items_table->horizontalHeader()->setSortIndicator(logicalIndex, Qt::AscendingOrder);
        ui->items_table->horizontalHeader()->setSortIndicatorShown(true);
    } else {
        proxyModel->sort(logicalIndex, Qt::DescendingOrder);
        ui->items_table->horizontalHeader()->setSortIndicator(logicalIndex, Qt::DescendingOrder);
        ui->items_table->horizontalHeader()->setSortIndicatorShown(true);
    }
}
