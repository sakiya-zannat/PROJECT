#include "quantitydelegate.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSqlQuery>
#include <QSqlError>
#include <QApplication>
#include <QMouseEvent>
#include <QTableView>
#include <qpainter.h>

QuantityDelegate::QuantityDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

bool QuantityDelegate::editorEvent(QEvent *event,
                                   QAbstractItemModel *model,
                                   const QStyleOptionViewItem &option,
                                   const QModelIndex &index)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QRect rect = option.rect.adjusted(2, 2, -2, -2);

        int buttonWidth = 25;
        int spacing = 4;
        QRect minusRect(rect.left(), rect.top(), buttonWidth, rect.height());
        QRect plusRect(rect.right() - buttonWidth, rect.top(), buttonWidth, rect.height());

        if (minusRect.contains(mouseEvent->pos())) {
            int currentQty = model->data(index).toInt();
            if (currentQty > 1) {
                int itemId = model->data(index.sibling(index.row(), 0)).toInt();
                emit quantityChanged(itemId, currentQty - 1);
            }
            return true;
        }
        else if (plusRect.contains(mouseEvent->pos())) {
            int currentQty = model->data(index).toInt();
            int itemId = model->data(index.sibling(index.row(), 0)).toInt();

            // Query current stock
            QSqlQuery query;
            query.prepare("SELECT stock FROM items WHERE id = ?");
            query.addBindValue(itemId);
            if (!query.exec() || !query.next()) {
                qDebug() << "Stock query failed:" << query.lastError().text();
                return true;
            }

            int stockQty = query.value(0).toInt();
            // Do NOT exceed stock
            if (currentQty < stockQty) {
                emit quantityChanged(itemId, currentQty + 1);
            }
            return true;
        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void QuantityDelegate::paint(QPainter *painter,
                             const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // Get the text
    QString quantityText = opt.text;

    // Clear the text from the option
    opt.text.clear();

    // Draw the item view item WITHOUT text
    const QWidget *widget = opt.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();

    // Draw background, selection, focus, etc. (without text)
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

    // Draw buttons and quantity
    QRect rect = opt.rect.adjusted(2, 2, -2, -2);
    int buttonWidth = 25;
    int spacing = 4;

    // Draw minus button
    QStyleOptionButton minusOpt;
    minusOpt.rect = QRect(rect.left(), rect.top(), buttonWidth, rect.height());
    minusOpt.text = "−";
    minusOpt.state = QStyle::State_Enabled;

    // Draw plus button
    QStyleOptionButton plusOpt;
    plusOpt.rect = QRect(rect.right() - buttonWidth, rect.top(), buttonWidth, rect.height());
    plusOpt.text = "+";
    plusOpt.state = QStyle::State_Enabled;

    // Draw quantity in middle
    QRect qtyRect(rect.left() + buttonWidth + spacing,
                  rect.top(),
                  rect.width() - 2 * buttonWidth - 2 * spacing,
                  rect.height());

    painter->save();
    QApplication::style()->drawControl(QStyle::CE_PushButton, &minusOpt, painter);
    QApplication::style()->drawControl(QStyle::CE_PushButton, &plusOpt, painter);
    painter->drawText(qtyRect, Qt::AlignCenter, quantityText);
    painter->restore();
}

QSize QuantityDelegate::sizeHint(const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(30); // Make sure buttons fit
    return size;
}
