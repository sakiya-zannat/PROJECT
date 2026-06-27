#pragma once

#include <QStyledItemDelegate>

class QuantityDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit QuantityDelegate(QObject *parent = nullptr);

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    bool editorEvent(QEvent *event,
                     QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;

signals:
    void quantityChanged(int itemId, int newQuantity);
};
