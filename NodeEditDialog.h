/**
 * @file NodeEditDialog.h
 * @brief 节点编辑对话框类头文件
 * @author
 * @version 1.0.0
 * @date 2024
 */

#ifndef NODEEDITDIALOG_H
#define NODEEDITDIALOG_H

#include <QDialog>
#include "NodeTemplate.h"

// 前向声明
class QLineEdit;
class QComboBox;
class QSpinBox;
class QTextEdit;
class QPushButton;
class QLabel;

/**
 * @class NodeEditDialog
 * @brief 节点模板编辑对话框，用于创建和编辑自定义节点
 */
class NodeEditDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 对话框模式
     */
    enum Mode {
        CreateMode,  ///< 创建新节点
        EditMode     ///< 编辑现有节点
    };

    /**
     * @brief 构造函数
     * @param mode 对话框模式
     * @param parent 父窗口
     */
    explicit NodeEditDialog(Mode mode = CreateMode, QWidget *parent = nullptr);

    /**
     * @brief 设置要编辑的节点模板
     * @param tmpl 节点模板
     */
    void setTemplate(const NodeTemplate &tmpl);

    /**
     * @brief 获取编辑后的节点模板
     * @return 节点模板
     */
    NodeTemplate getTemplate() const;

private slots:
    /**
     * @brief 选择颜色按钮点击事件
     */
    void onSelectColor();

    /**
     * @brief 验证输入并接受对话框
     */
    void onAccept();

private:
    /**
     * @brief 设置用户界面
     */
    void setupUI();

    /**
     * @brief 更新颜色预览
     */
    void updateColorPreview();

    Mode m_mode;                    ///< 对话框模式
    QColor m_selectedColor;         ///< 选中的颜色

    // UI组件
    QLineEdit *m_typeIdEdit;        ///< 类型ID输入框
    QLineEdit *m_displayNameEdit;   ///< 显示名称输入框
    QComboBox *m_categoryCombo;     ///< 分类下拉框
    QPushButton *m_colorButton;     ///< 颜色选择按钮
    QLabel *m_colorPreview;         ///< 颜色预览标签
    QSpinBox *m_inputPortSpin;      ///< 输入端口数量
    QSpinBox *m_outputPortSpin;     ///< 输出端口数量
    QTextEdit *m_descriptionEdit;   ///< 描述文本框
    QLineEdit *m_paramsEdit;        ///< 默认参数输入框
};

#endif // NODEEDITDIALOG_H

