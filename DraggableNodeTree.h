/**
 * @file DraggableNodeTree.h
 * @brief 可拖拽的节点树控件
 * @author
 * @version 1.0.0
 * @date 2024
 */

#ifndef DRAGGABLENODETREE_H
#define DRAGGABLENODETREE_H

#include <QTreeWidget>
#include <QMimeData>
#include <QDrag>

/**
 * @class DraggableNodeTree
 * @brief 支持拖拽节点到画布的树形控件
 * 
 * 继承自 QTreeWidget，提供自定义 MIME 数据以支持将节点拖拽到画布
 */
class DraggableNodeTree : public QTreeWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父控件指针
     */
    explicit DraggableNodeTree(QWidget *parent = nullptr);

protected:
    /**
     * @brief 生成拖拽的 MIME 数据
     * @param items 被拖拽的项目列表
     * @return 包含节点类型信息的 MIME 数据
     */
    QMimeData *mimeData(const QList<QTreeWidgetItem *> &items) const override;

    /**
     * @brief 返回支持的 MIME 类型列表
     * @return MIME 类型字符串列表
     */
    QStringList mimeTypes() const override;

    /**
     * @brief 开始拖拽操作
     * @param supportedActions 支持的操作类型
     */
    void startDrag(Qt::DropActions supportedActions) override;
};

#endif // DRAGGABLENODETREE_H

