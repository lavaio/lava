#include "uiexception.h"
#include <QMessageBox>

UiException::UiException(UiException::Level level, const QString &title, const QString &message, QWidget* parent):
    _level(level), _title(title), _message(message), _parent(parent)
{

}

void UiException::showAlertIfNecessary()
{
    switch(_level) {
    case Information:
        QMessageBox::information(_parent, _title, _message, QMessageBox::Ok, QMessageBox::Ok);
        break;
    case Warning:
        QMessageBox::warning(_parent, _title, _message, QMessageBox::Ok, QMessageBox::Ok);
        break;
    case Error:
        QMessageBox::critical(_parent, _title, _message, QMessageBox::Ok, QMessageBox::Ok);
        break;
    }
}
