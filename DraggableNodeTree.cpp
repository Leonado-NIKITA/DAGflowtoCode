/**
 * @file DraggableNodeTree.cpp
 * @brief 可拖拽的节点树控件实现
 * @author
 * @version 1.0.0
 * @date 2024
 */

#include "DraggableNodeTree.h"
#include <QApplication>
#include <QPixmap>
#include <QPainter>

/**
 * @brief 构造函数
 * @param parent 父控件指针
 */
DraggableNodeTree::DraggableNodeTree(QWidget *parent)
    : QTreeWidget(parent)
{
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
}

/**
 * @brief 返回支持的 MIME 类型列表
 * @return MIME 类型字符串列表
 */
QStringList DraggableNodeTree::mimeTypes() const
{
    QStringList types;
    types << "application/x-nodetype";
    return types;
}

/**
 * @brief 生成拖拽的 MIME 数据
 * @param items 被拖拽的项目列表
 * @return 包含节点类型信息的 MIME 数据
 */
QMimeData *DraggableNodeTree::mimeData(const QList<QTreeWidgetItem *> &items) const
{
    if (items.isEmpty()) {
        return nullptr;
    }

    QTreeWidgetItem *item = items.first();
    
    // 只允许拖拽叶子节点（实际的节点类型）
    if (item->childCount() > 0) {
        return nullptr;
    }

    // 获取节点类型 ID
    QString nodeType = item->data(0, Qt::UserRole).toString();
    if (nodeType.isEmpty()) {
        return nullptr;
    }

    QMimeData *mimeData = new QMimeData();
    mimeData->setData("application/x-nodetype", nodeType.toUtf8());
    mimeData->setText(item->text(0));  // 设置显示文本用于预览
    
    return mimeData;
}

/**
 * @brief 开始拖拽操作
 * @param supportedActions 支持的操作类型
 */
void DraggableNodeTree::startDrag(Qt::DropActions supportedActions)
{
    QList<QTreeWidgetItem *> items = selectedItems();
    if (items.isEmpty()) {
        return;
    }

    QTreeWidgetItem *item = items.first();
    
    // 只允许拖拽叶子节点
    if (item->childCount() > 0) {
        return;
    }

    // 获取节点类型
    QString nodeType = item->data(0, Qt::UserRole).toString();
    if (nodeType.isEmpty()) {
        return;
    }

    QMimeData *data = mimeData(items);
    if (!data) {
        return;
    }

    QDrag *drag = new QDrag(this);
    drag->setMimeData(data);

    // 创建拖拽预览图像
    QPixmap pixmap(120, 40);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制节点预览
    QLinearGradient gradient(0, 0, 120, 40);
    gradient.setColorAt(0, QColor(100, 150, 200, 200));
    gradient.setColorAt(1, QColor(60, 100, 150, 200));
    
    painter.setBrush(gradient);
    painter.setPen(QPen(QColor(50, 80, 120), 2));
    painter.drawRoundedRect(2, 2, 116, 36, 8, 8);
    
    // 绘制节点名称
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, item->text(0));
    
    painter.end();

    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));

    drag->exec(supportedActions, Qt::CopyAction);
}

