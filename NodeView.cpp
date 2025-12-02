/**
 * @file NodeView.cpp
 * @brief 节点视图类实现文件
 * @author 
 * @version 1.0.0
 * @date 2024
 */

#include "NodeView.h"
#include "NodeScene.h"
#include "MiniMapWidget.h"
#include "GroupNode.h"
#include "Node.h"
#include "NodeLibrary.h"
#include "NodeEditDialog.h"
#include <QWheelEvent>     // 鼠标滚轮事件类
#include <QKeyEvent>       // 键盘事件类
#include <QResizeEvent>    // 窗口大小改变事件类
#include <QMouseEvent>     // 鼠标事件类
#include <QScrollBar>      // 滚动条类
#include <QDragEnterEvent> // 拖拽进入事件类
#include <QDragMoveEvent>  // 拖拽移动事件类
#include <QDropEvent>      // 拖拽释放事件类
#include <QMimeData>       // MIME数据类
#include <QMenu>           // 菜单类
#include <QContextMenuEvent> // 上下文菜单事件类
#include <QMessageBox>     // 消息框类

/**
 * @brief 构造函数
 * @param scene 要显示的节点场景
 * @param parent 父控件指针，默认为nullptr
 */
NodeView::NodeView(NodeScene *scene, QWidget *parent)
    : QGraphicsView(parent)     // 初始化基类
    , m_scene(scene)            // 保存场景指针
    , m_miniMap(nullptr)        // 初始化小地图指针
    , m_isPanning(false)        // 初始化拖动状态
{
    // 设置关联的场景
    setScene(scene);
    
    // 启用抗锯齿以提高绘制质量
    setRenderHint(QPainter::Antialiasing);
    
    // 设置拖拽模式为橡皮筋选择
    setDragMode(QGraphicsView::RubberBandDrag);
    
    // 设置视口更新模式为全量更新，确保显示效果
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    // 设置变换锚点为鼠标位置，使缩放操作更直观
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    
    // 设置视图的最小尺寸
    setMinimumSize(400, 300);
    
    // 启用拖放功能
    setAcceptDrops(true);
    
    // 创建导航小地图（作为子控件显示在右下角）
    m_miniMap = new MiniMapWidget(this);
    m_miniMap->setSceneAndView(scene, this);
}

/**
 * @brief 鼠标滚轮事件处理
 * @param event 鼠标滚轮事件对象
 * 
 * 使用鼠标滚轮进行视图缩放，滚轮向上放大，向下缩小
 */
void NodeView::wheelEvent(QWheelEvent *event)
{
    // 计算缩放因子，每120度滚轮角度对应2的幂次缩放
    scaleView(std::pow(2.0, event->angleDelta().y() / 240.0));
}

/**
 * @brief 背景绘制事件处理
 * @param painter 绘制器对象
 * @param rect 需要重绘的矩形区域
 * 
 * 绘制深色背景和网格线，提供更好的视觉参考
 */
void NodeView::drawBackground(QPainter *painter, const QRectF &rect)
{
    // 绘制深色背景
    painter->fillRect(rect, QColor(30, 30, 40));
    
    // 绘制网格线
    painter->setPen(QPen(QColor(60, 60, 70), 1));
    
    // 计算网格起始位置，确保网格对齐
    qreal left = int(rect.left()) - (int(rect.left()) % 20);
    qreal top = int(rect.top()) - (int(rect.top()) % 20);
    
    // 绘制垂直网格线
    for (qreal x = left; x < rect.right(); x += 20) {
        painter->drawLine(x, rect.top(), x, rect.bottom());
    }
    
    // 绘制水平网格线
    for (qreal y = top; y < rect.bottom(); y += 20) {
        painter->drawLine(rect.left(), y, rect.right(), y);
    }
}

/**
 * @brief 键盘按键事件处理
 * @param event 键盘事件对象
 * 
 * 支持以下快捷键：
 * - '+': 放大视图
 * - '-': 缩小视图  
 * - 'Delete': 删除选中项
 * - 'Ctrl+C': 复制选中项
 * - 'Ctrl+V': 粘贴
 * - 'Ctrl+X': 剪切选中项
 * - 'Ctrl+A': 全选
 * - 'Escape': 取消当前操作
 */
void NodeView::keyPressEvent(QKeyEvent *event)
{
    // 处理带 Ctrl 修饰键的快捷键
    if (event->modifiers() & Qt::ControlModifier) {
        switch (event->key()) {
        case Qt::Key_C:  // Ctrl+C: 复制
            m_scene->copySelected();
            return;
        case Qt::Key_V:  // Ctrl+V: 粘贴
            if (m_scene->canPaste()) {
                // 在视图中心粘贴
                QPointF center = mapToScene(viewport()->rect().center());
                m_scene->paste(center);
            }
            return;
        case Qt::Key_X:  // Ctrl+X: 剪切
            m_scene->cutSelected();
            return;
        case Qt::Key_A:  // Ctrl+A: 全选
            m_scene->selectAll();
            return;
        case Qt::Key_Z:  // Ctrl+Z: 撤销
            m_scene->undoStack()->undo();
            return;
        case Qt::Key_Y:  // Ctrl+Y: 重做
            m_scene->undoStack()->redo();
            return;
        default:
            break;
        }
    }
    
    // Ctrl+Shift+Z 也作为重做
    if ((event->modifiers() & Qt::ControlModifier) && (event->modifiers() & Qt::ShiftModifier)) {
        if (event->key() == Qt::Key_Z) {
            m_scene->undoStack()->redo();
            return;
        }
    }
    
    // 处理不带修饰键的快捷键
    switch (event->key()) {
    case Qt::Key_Plus:        // 加号键：放大
    case Qt::Key_Equal:       // = 键（带 Shift 为 +）
        scaleView(1.2);
        break;
    case Qt::Key_Minus:       // 减号键：缩小
        scaleView(1.0 / 1.2);
        break;
    case Qt::Key_Delete:      // Delete键：删除选中项
    case Qt::Key_Backspace:   // Backspace键：删除选中项
        m_scene->deleteSelected();
        break;
    case Qt::Key_Escape:      // Escape键：取消当前操作
        if (m_scene->getConnectionState() != NodeScene::None) {
            m_scene->cancelConnection();
        } else {
            m_scene->clearSelection();
        }
        break;
    default:
        // 其他按键交给基类处理
        QGraphicsView::keyPressEvent(event);
    }
}

/**
 * @brief 窗口大小改变事件处理
 * @param event 大小改变事件对象
 */
void NodeView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    updateMiniMapPosition();
}

/**
 * @brief 缩放视图
 * @param scaleFactor 缩放因子
 * 
 * 安全地缩放视图，确保缩放比例在合理范围内（0.07到100之间）
 */
void NodeView::scaleView(qreal scaleFactor)
{
    // 计算应用缩放后的比例因子
    qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    
    // 检查缩放范围是否合理
    if (factor < 0.07 || factor > 100) return;
    
    // 应用缩放变换
    scale(scaleFactor, scaleFactor);
    
    // 更新小地图
    if (m_miniMap) {
        m_miniMap->updateMiniMap();
    }
}

/**
 * @brief 更新小地图位置
 * 
 * 将小地图固定在视图的右下角
 */
void NodeView::updateMiniMapPosition()
{
    if (m_miniMap) {
        const int margin = 10;
        int x = width() - m_miniMap->width() - margin;
        int y = height() - m_miniMap->height() - margin;
        m_miniMap->move(x, y);
        m_miniMap->raise();  // 确保小地图在最上层
    }
}

/**
 * @brief 鼠标按下事件处理
 * @param event 鼠标事件对象
 * 
 * 右键按下在空白区域时开始拖动画布
 * 连接过程中禁用框选
 */
void NodeView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        // 检查是否点击在空白区域（没有图形项）
        QGraphicsItem *item = itemAt(event->pos());
        if (!item) {
            // 开始拖动画布
            m_isPanning = true;
            m_panStartPos = event->pos();
            setCursor(Qt::ClosedHandCursor);
        }
        // 右键点击时不传递给基类，避免取消选择
        event->accept();
        return;
    }
    
    // 在连接过程中禁用框选模式
    if (event->button() == Qt::LeftButton && 
        m_scene->getConnectionState() != NodeScene::None) {
        // 连接过程中，临时设置为无拖拽模式以避免框选
        setDragMode(QGraphicsView::NoDrag);
    }
    
    QGraphicsView::mousePressEvent(event);
}

/**
 * @brief 鼠标移动事件处理
 * @param event 鼠标事件对象
 * 
 * 右键拖动时移动画布
 * 连接过程中禁用框选
 */
void NodeView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isPanning) {
        // 计算移动距离
        QPoint delta = event->pos() - m_panStartPos;
        m_panStartPos = event->pos();
        
        // 移动画布（通过调整滚动条位置）
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        
        // 更新小地图
        if (m_miniMap) {
            m_miniMap->updateMiniMap();
        }
        
        event->accept();
        return;
    }
    
    // 在连接过程中，确保框选模式被禁用
    if (m_scene->getConnectionState() != NodeScene::None) {
        if (dragMode() != QGraphicsView::NoDrag) {
            setDragMode(QGraphicsView::NoDrag);
        }
    }
    
    QGraphicsView::mouseMoveEvent(event);
}

/**
 * @brief 鼠标释放事件处理
 * @param event 鼠标事件对象
 * 
 * 右键释放停止拖动画布
 * 左键释放恢复框选模式
 */
void NodeView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton && m_isPanning) {
        // 停止拖动画布
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    
    // 右键取消连接后恢复框选模式
    if (event->button() == Qt::RightButton) {
        // 连接被取消，恢复框选模式
        if (dragMode() != QGraphicsView::RubberBandDrag) {
            setDragMode(QGraphicsView::RubberBandDrag);
        }
    }
    
    // 左键释放时，如果不在连接过程中，恢复框选模式
    if (event->button() == Qt::LeftButton) {
        if (m_scene->getConnectionState() == NodeScene::None && 
            dragMode() != QGraphicsView::RubberBandDrag) {
            setDragMode(QGraphicsView::RubberBandDrag);
        }
    }
    
    QGraphicsView::mouseReleaseEvent(event);
}

/**
 * @brief 拖拽进入事件处理
 * @param event 拖拽事件对象
 * 
 * 检查拖拽的数据是否为节点类型
 */
void NodeView::dragEnterEvent(QDragEnterEvent *event)
{
    // 检查是否包含节点类型数据
    if (event->mimeData()->hasFormat("application/x-nodetype")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

/**
 * @brief 拖拽移动事件处理
 * @param event 拖拽事件对象
 */
void NodeView::dragMoveEvent(QDragMoveEvent *event)
{
    // 检查是否包含节点类型数据
    if (event->mimeData()->hasFormat("application/x-nodetype")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

/**
 * @brief 拖拽释放事件处理
 * @param event 拖拽事件对象
 * 
 * 在释放位置创建新节点
 */
void NodeView::dropEvent(QDropEvent *event)
{
    // 检查是否包含节点类型数据
    if (event->mimeData()->hasFormat("application/x-nodetype")) {
        // 获取节点类型
        QString nodeType = QString::fromUtf8(event->mimeData()->data("application/x-nodetype"));
        
        // 将视图坐标转换为场景坐标
        QPointF scenePos = mapToScene(event->position().toPoint());
        
        // 在场景中添加节点
        m_scene->addNode(nodeType, scenePos);
        
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

/**
 * @brief 右键菜单事件处理
 * @param event 上下文菜单事件对象
 * 
 * 显示右键上下文菜单，包含打包、拆分、复制、粘贴、删除等操作
 */
void NodeView::contextMenuEvent(QContextMenuEvent *event)
{
    // 如果正在拖动画布，不显示菜单
    if (m_isPanning) {
        return;
    }
    
    // 获取点击位置的图形项
    QGraphicsItem *item = itemAt(event->pos());
    
    // 创建右键菜单
    QMenu contextMenu(this);
    
    // 获取选中的项目
    QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
    
    // 检查是否选中了单个节点（用于编辑模板）
    Node *selectedNode = nullptr;
    if (selectedItems.size() == 1) {
        selectedNode = dynamic_cast<Node*>(selectedItems.first());
    }
    
    // 编辑操作
    QAction *copyAction = contextMenu.addAction("复制");
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setEnabled(!selectedItems.isEmpty());
    
    QAction *cutAction = contextMenu.addAction("剪切");
    cutAction->setShortcut(QKeySequence::Cut);
    cutAction->setEnabled(!selectedItems.isEmpty());
    
    QAction *pasteAction = contextMenu.addAction("粘贴");
    pasteAction->setShortcut(QKeySequence::Paste);
    pasteAction->setEnabled(m_scene->canPaste());
    
    contextMenu.addSeparator();
    
    // 打包/拆分操作
    QAction *groupAction = contextMenu.addAction("打包节点");
    groupAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
    groupAction->setEnabled(m_scene->canGroup());
    
    QAction *ungroupAction = contextMenu.addAction("拆分节点");
    ungroupAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G));
    ungroupAction->setEnabled(m_scene->canUngroup());
    
    contextMenu.addSeparator();
    
    // 节点模板编辑操作（仅当选中单个节点且不是组节点时可用）
    QAction *editTemplateAction = contextMenu.addAction("编辑节点模板...");
    bool canEditTemplate = selectedNode && !dynamic_cast<GroupNode*>(selectedNode);
    editTemplateAction->setEnabled(canEditTemplate);
    
    contextMenu.addSeparator();
    
    // 删除操作
    QAction *deleteAction = contextMenu.addAction("删除");
    deleteAction->setShortcut(QKeySequence::Delete);
    deleteAction->setEnabled(!selectedItems.isEmpty());
    
    contextMenu.addSeparator();
    
    // 全选操作
    QAction *selectAllAction = contextMenu.addAction("全选");
    selectAllAction->setShortcut(QKeySequence::SelectAll);
    
    // 显示菜单并处理选择
    QAction *selectedAction = contextMenu.exec(event->globalPos());
    
    if (selectedAction == copyAction) {
        m_scene->copySelected();
    } else if (selectedAction == cutAction) {
        m_scene->cutSelected();
    } else if (selectedAction == pasteAction) {
        // 在点击位置粘贴
        QPointF scenePos = mapToScene(event->pos());
        m_scene->paste(scenePos);
    } else if (selectedAction == groupAction) {
        m_scene->groupSelected();
    } else if (selectedAction == ungroupAction) {
        m_scene->ungroupSelected();
    } else if (selectedAction == editTemplateAction && selectedNode) {
        // 编辑节点模板
        QString typeId = selectedNode->getType();
        NodeTemplate tmpl = NodeLibrary::instance()->getTemplate(typeId);
        
        if (tmpl.isValid()) {
            NodeEditDialog dialog(NodeEditDialog::EditMode, this);
            dialog.setTemplate(tmpl);
            
            if (dialog.exec() == QDialog::Accepted) {
                NodeTemplate updatedTmpl = dialog.getTemplate();
                if (NodeLibrary::instance()->updateTemplate(updatedTmpl)) {
                    // 更新成功，节点库会自动通知场景更新相关节点
                } else {
                    QMessageBox::warning(this, "更新失败", "无法更新节点模板");
                }
            }
        } else {
            QMessageBox::warning(this, "错误", "无法获取节点模板信息");
        }
    } else if (selectedAction == deleteAction) {
        m_scene->deleteSelected();
    } else if (selectedAction == selectAllAction) {
        m_scene->selectAll();
    }
}