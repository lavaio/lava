#ifndef UIEXCEPTION_H
#define UIEXCEPTION_H

#include <QString>

QT_BEGIN_NAMESPACE
class QString;
class QWidget;
QT_END_NAMESPACE


class UiException
{
public:
    enum Level {
        Information = 0,
        Warning,
        Error,
    };

    UiException(Level level, const QString& title, const QString& message, QWidget* parent);
    void showAlertIfNecessary();

private:
    Level _level;
    QString _title;
    QString _message;
    QWidget* _parent;
};

#endif // UIEXCEPTION_H
