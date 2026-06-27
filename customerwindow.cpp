#include "customerwindow.h"
#include "mainwindow.h"
#include "orderitemsdialog.h"
#include "quantitydelegate.h"
#include "ui_customerwindow.h"
#include <QSortFilterProxyModel>
#include <QSqlQuery>
#include <QMessageBox>
#include <QSqlError>

CustomerWindow::CustomerWindow(int usrId, QSqlTableModel *sharedModel, QSqlRelationalTableModel *sharedOrdersModel, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CustomerWindow)
    , userId(usrId)
    , itemsModel(sharedModel)
    , ordersModel(sharedOrdersModel)
{
    ui->setupUi(this);

    // Fetch username from database
    QString username;
    QSqlQuery query;
    query.prepare("SELECT username FROM users WHERE id = ?");
    query.addBindValue(userId);
    if (query.exec() && query.next()) {
        username = query.value(0).toString();
    }
    setWindowTitle("Customer: " + username);

    itemsProxy = new QSortFilterProxyModel(this);
    itemsProxy->setSourceModel(itemsModel);
    // Case-insensitive search
    itemsProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    // Filter by the "name" column
    itemsProxy->setFilterKeyColumn(itemsModel->fieldIndex("name"));

    ui->items_table->setModel(itemsProxy);
    ui->items_table->hideColumn(itemsModel->fieldIndex("id"));
    ui->items_table->hideColumn(itemsModel->fieldIndex("stock"));
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

    cartModel = new QSqlQueryModel(this);

    refreshCart();

    ui->cart_table->setModel(cartModel);
    ui->cart_table->hideColumn(0);
    ui->cart_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->cart_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->cart_table->setFocusPolicy(Qt::NoFocus);
    ui->cart_table->setStyleSheet(R"(
        QTableView::item:selected {
            background-color: #d0eaff;
            color: black;
        }
    )");

    header = ui->cart_table->horizontalHeader();
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    // header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    NumericDelegate *numericDelegate2 = new NumericDelegate(ui->cart_table);
    ui->cart_table->setItemDelegateForColumn(2, numericDelegate2);
    ui->cart_table->setItemDelegateForColumn(4, numericDelegate2);

    QuantityDelegate *qtyDelegate = new QuantityDelegate(this);
    ui->cart_table->setItemDelegateForColumn(3, qtyDelegate);
    connect(qtyDelegate, &QuantityDelegate::quantityChanged,
            this, &CustomerWindow::handleQuantityChange);

    connect(ui->items_table->horizontalHeader(), &QHeaderView::sectionClicked,
            this, &CustomerWindow::onHeaderClicked);
    connect(ui->search_bar, &QLineEdit::textChanged, this, [=](const QString &text){
        itemsProxy->setFilterFixedString(text);
    });
    connect(ui->add2cart_btn, &QPushButton::clicked,
            this, &CustomerWindow::addToCart);
    connect(ui->rem_cart_btn, &QPushButton::clicked,
            this, &CustomerWindow::removeFromCart);

    ui->total_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->total_label->setStyleSheet(R"(
    QLabel {
        font-weight: bold;
        font-size: 14px;
        background-color: #f9f9f9;   /* subtle table-like bg */
        border-top: 1px solid #d0eaff; /* similar to selection highlight */
        padding: 4px 8px;
    }
    )");

    connect(ui->checkout_btn, &QPushButton::clicked,
            this, &CustomerWindow::checkout);

    ordersProxy = new QSortFilterProxyModel(this);
    ordersProxy->setSourceModel(ordersModel);
    ordersProxy->setFilterKeyColumn(ordersModel->fieldIndex("user_id"));
    ordersProxy->setFilterFixedString(username);
    ordersProxy->setDynamicSortFilter(true);
    ordersProxy->sort(ordersModel->fieldIndex("id"), Qt::DescendingOrder);
    ui->orders_table->setModel(ordersProxy);
    ui->orders_table->hideColumn(ordersModel->fieldIndex("username"));

    ui->orders_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->orders_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->orders_table->setFocusPolicy(Qt::NoFocus);
    ui->orders_table->setStyleSheet(R"(
        QTableView::item:selected {
            background-color: #d0eaff;
            color: black;
        }
        )");

    int idCol = ordersModel->fieldIndex("id");
    int timeCol = ordersModel->fieldIndex("created_at");
    int statusCol   = ordersModel->fieldIndex("status");

    header = ui->orders_table->horizontalHeader();
    header->setSectionResizeMode(idCol, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(timeCol, QHeaderView::Stretch);
    header->setSectionResizeMode(statusCol, QHeaderView::ResizeToContents);

    ui->orders_table->setToolTip("Double-click an order to view details");
    connect(ui->orders_table,&QTableView::doubleClicked, this, &CustomerWindow::onOrderRowClicked);

}

void CustomerWindow::onOrderRowClicked(const QModelIndex &proxyIndex)
{
    if (!proxyIndex.isValid())
        return;

    int idCol = ordersModel->fieldIndex("id");
    QModelIndex sourceIndex = ordersProxy->mapToSource(proxyIndex);
    int orderId = sourceIndex
                      .siblingAtColumn(idCol)
                      .data()
                      .toInt();

    OrderItemsDialog dlg(orderId, this);
    dlg.exec();
}

void CustomerWindow::checkout()
{
    if (!cartModel || cartModel->rowCount() == 0) {
        QMessageBox::information(this, "Checkout", "Your cart is empty!");
        return;
    }

    QSqlQuery stockQuery;
    bool hasError = false;
    QStringList problems;

    for (int row = 0; row < cartModel->rowCount(); ++row) {

        int itemId = cartModel->data(cartModel->index(row, 0)).toInt();
        QString name = cartModel->data(cartModel->index(row, 1)).toString();
        int cartQty = cartModel->data(cartModel->index(row, 3)).toInt();

        stockQuery.prepare(
            "SELECT stock FROM items WHERE id = ?"
            );
        stockQuery.addBindValue(itemId);

        if (!stockQuery.exec() || !stockQuery.next()) {
            QString msg = name + ": unable to verify stock";
            problems << msg;
            hasError = true;
            continue;
        }

        int stockQty = stockQuery.value(0).toInt();

        if (!stockQty) {
            QString msg = QString(
                              "%1: Out of stock (remove item)"
                              ).arg(name);
        } else if (cartQty > stockQty) {
            QString msg = QString(
                              "%1: Only %2 in stock (reduce quantity)"
                              ).arg(name).arg(stockQty);

            problems << msg;
            hasError = true;
        }
    }

    if (hasError) {
        QMessageBox::warning(
            this,
            "Stock issue",
            "Please fix the following items before checkout:\n\n"
                + problems.join("\n")
            );
        return; // 🚫 STOP
    }

    // ✅ All good → continue checkout
    // QMessageBox::information(this, "Checkout", "All items are in stock!");
    // =======================
    // 1. Start transaction
    // =======================
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) {
        QMessageBox::critical(this, "Error", "Failed to start transaction");
        return;
    }

    // =======================
    // 2. Create order
    // =======================
    QSqlQuery orderQuery;
    orderQuery.prepare(
        "INSERT INTO orders (user_id, created_at, status) "
        "VALUES (?, datetime('now'), 'Pending')"
        );
    orderQuery.addBindValue(userId);

    if (!orderQuery.exec()) {
        db.rollback();
        QMessageBox::critical(this, "Error", "Failed to create order");
        return;
    }

    int orderId = orderQuery.lastInsertId().toInt();

    // =======================
    // 3. Create order items
    // =======================


    for (int row = 0; row < cartModel->rowCount(); ++row) {

        int itemId = cartModel->data(cartModel->index(row, 0)).toInt();
        int qty    = cartModel->data(cartModel->index(row, 3)).toInt();

        QSqlQuery itemQuery;
        // Insert order item
        itemQuery.prepare(
            "INSERT INTO order_items "
            "(order_id, item_id, quantity) "
            "VALUES (?, ?, ?)"
            );
        itemQuery.addBindValue(orderId);
        itemQuery.addBindValue(itemId);
        itemQuery.addBindValue(qty);

        if (!itemQuery.exec()) {
            db.rollback();
            qDebug() << "Error inserting item:" << itemQuery.lastError().text();
            QMessageBox::critical(this, "Error", "Failed to create order items");
            return;
        }
    }

    // =======================
    // 4. Clear cart
    // =======================
    QSqlQuery clearCartQuery;
    clearCartQuery.prepare(
        "DELETE FROM cart_items "
        "WHERE user_id = ?"
        );
    clearCartQuery.addBindValue(userId);

    if (!clearCartQuery.exec()) {
        db.rollback();
        QMessageBox::critical(this, "Error", "Failed to clear cart");
        return;
    }

    // =======================
    // 5. Commit transaction
    // =======================
    if (!db.commit()) {
        QMessageBox::critical(this, "Error", "Checkout failed");
        return;
    }

    // =======================
    // 6. Refresh UI
    // =======================
    refreshCart();
    ordersModel->select();

    QMessageBox::information(
        this,
        "Checkout complete",
        QString("Order #%1 created successfully!").arg(orderId)
    );

}

void CustomerWindow::updateCartTotal() // Pass the user ID here
{
    QSqlQuery query;
    // 1. Use prepare() for queries with '?'
    query.prepare(
        "SELECT SUM(i.price * c.quantity) "
        "FROM cart_items c "
        "JOIN items i ON i.id = c.item_id "
        "WHERE c.user_id = ?"
        );

    // 2. Bind the actual value
    query.addBindValue(userId);

    double total = 0.0;

    // 3. Execute and check for success
    if (query.exec()) {
        if (query.next() && !query.value(0).isNull()) {
            total = query.value(0).toDouble();
        }
    } else {
        qDebug() << "Total calculation error:" << query.lastError().text();
    }

    // Format with BDT and two decimals
    ui->total_label->setText(QString("Cart Total: BDT %1").arg(total, 0, 'f', 2));
}

void CustomerWindow::handleQuantityChange(int itemId, int newQuantity)
{
    QSqlQuery query;

    if (newQuantity > 0) {
        query.prepare("UPDATE cart_items SET quantity = ? WHERE user_id = ? AND item_id = ?");
        query.addBindValue(newQuantity);
    }
    query.addBindValue(userId);
    query.addBindValue(itemId);

    if (!query.exec()) {
        qDebug() << "Database error:" << query.lastError().text();
        return;
    }

    // Only refresh if update was successful
    refreshCart();
}

void CustomerWindow::refreshCart()
{
    QSqlQuery query;
    query.prepare(
        "SELECT "
        "items.id, "
        "items.name, "
        "items.price, "
        "cart_items.quantity, "
        "(items.price * cart_items.quantity) AS total "
        "FROM cart_items "
        "JOIN items ON items.id = cart_items.item_id "
        "WHERE cart_items.user_id = ?"
        );

    // Bind your variable here
    query.addBindValue(userId);

    // Execute the query
    if (query.exec()) {
        // Pass the executed query object to the model
        cartModel->setQuery(std::move(query));
    } else {
        qDebug() << "Query failed:" << query.lastError().text();
    }
    updateCartTotal();
}

void CustomerWindow::removeFromCart()
{
    QModelIndexList selectedRows =
        ui->cart_table->selectionModel()->selectedRows();

    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "Remove from Cart",
                             "Please select at least one item to remove.");
        return;
    }

    if (QMessageBox::question(this, "Remove Items",
                              QString("Remove %1 item(s) from cart?")
                                  .arg(selectedRows.count()))
        != QMessageBox::Yes)
        return;

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);

    db.transaction();   // 🔒 important

    query.prepare(
        "DELETE FROM cart_items "
        "WHERE user_id = :u_id "
        "AND item_id = :i_id");
    query.bindValue(":u_id", userId);

    for (const QModelIndex &index : std::as_const(selectedRows)) {
        int row = index.row();
        int itemId = cartModel->data(cartModel->index(row, 0)).toInt();

        query.bindValue(":i_id", itemId);

        if (!query.exec()) {
            db.rollback();
            QMessageBox::critical(this, "Error", query.lastError().text());
            return;
        }
    }

    db.commit();        // ✅ all-or-nothing
    refreshCart();
}

void CustomerWindow::addToCart()
{
    QModelIndexList selectedRows =
        ui->items_table->selectionModel()->selectedRows();

    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "Add to Cart",
                             "Please select at least one item to add.");
        return;
    }

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);
    QSqlQuery stockQuery(db);

    db.transaction();

    QStringList problems; // messages like checkout()

    int idCol   = itemsModel->fieldIndex("id");
    int nameCol = itemsModel->fieldIndex("name");

    for (const QModelIndex &proxyIndex : std::as_const(selectedRows)) {

        QModelIndex index = itemsProxy->mapToSource(proxyIndex);
        int row = index.row();

        int itemId   = itemsModel->data(itemsModel->index(row, idCol)).toInt();
        QString name = itemsModel->data(itemsModel->index(row, nameCol)).toString();

        // --- DB check for current stock ---
        stockQuery.prepare("SELECT stock FROM items WHERE id = ?");
        stockQuery.addBindValue(itemId);

        if (!stockQuery.exec() || !stockQuery.next()) {
            problems << name + ": unable to verify stock";
            continue; // skip adding
        }

        int dbStockQty = stockQuery.value(0).toInt();

        if (dbStockQty <= 0) {
            problems << QString("%1: Out of stock").arg(name);
            continue; // skip adding
        }

        // --- DB check for current cart quantity ---
        int currentQty = 0;
        stockQuery.prepare("SELECT quantity FROM cart_items WHERE user_id = ? AND item_id = ?");
        stockQuery.addBindValue(userId);
        stockQuery.addBindValue(itemId);
        if (stockQuery.exec() && stockQuery.next()) {
            currentQty = stockQuery.value(0).toInt();
        }

        if (currentQty >= dbStockQty) {
            problems << QString("%1: Already at maximum stock (%2)").arg(name).arg(dbStockQty);
            continue; // cannot add more
        }

        int addQty = qMin(1, dbStockQty - currentQty);

        query.prepare(
            "INSERT INTO cart_items (user_id, item_id, quantity) "
            "VALUES (:u_id, :i_id, :qty) "
            "ON CONFLICT(user_id, item_id) DO UPDATE SET quantity = quantity + :qty"
            );
        query.bindValue(":u_id", userId);
        query.bindValue(":i_id", itemId);
        query.bindValue(":qty", addQty);

        if (!query.exec()) {
            db.rollback();
            QMessageBox::critical(this, "Error", query.lastError().text());
            return;
        }
    }

    db.commit();
    refreshCart();

    if (!problems.isEmpty()) {
        QMessageBox::warning(
            this,
            "Add to Cart - Stock Issue",
            "Some items could not be added to the cart:\n\n" + problems.join("\n")
            );
    }
}

void CustomerWindow::onHeaderClicked(int logicalIndex)
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
        itemsProxy->sort(-1); // reset sorting
        ui->items_table->horizontalHeader()->setSortIndicatorShown(false);
    } else if (currentSortMode == Ascending) {
        itemsProxy->sort(logicalIndex, Qt::AscendingOrder);
        ui->items_table->horizontalHeader()->setSortIndicator(logicalIndex, Qt::AscendingOrder);
        ui->items_table->horizontalHeader()->setSortIndicatorShown(true);
    } else {
        itemsProxy->sort(logicalIndex, Qt::DescendingOrder);
        ui->items_table->horizontalHeader()->setSortIndicator(logicalIndex, Qt::DescendingOrder);
        ui->items_table->horizontalHeader()->setSortIndicatorShown(true);
    }
}

CustomerWindow::~CustomerWindow()
{
    delete ui;
}
