#ifndef FILECONTENTDIALOG_H
#define FILECONTENTDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>

class FileContentDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FileContentDialog(const QString &fileName, QWidget *parent = nullptr);
    QString content() const;

signals:
    void fileSaved(const QString &fileName, int bytesWritten);

private slots:
    void onSave();
    void onTruncate();
    void onRefresh();

private:
    void loadContent();
    void setupUI();
    void applyStyles();

    QString m_fileName;
    QPlainTextEdit *m_editor;
    QLabel *m_infoLabel;
    QPushButton *m_saveBtn;
    QPushButton *m_truncateBtn;
    QPushButton *m_refreshBtn;
};

#endif
