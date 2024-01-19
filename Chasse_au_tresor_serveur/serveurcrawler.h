#ifndef SERVEURCRAWLER_H
#define SERVEURCRAWLER_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QGridLayout>
#include <QPoint>
#include <QList>
#define TAILLE 20
#include <QToolButton>

QT_BEGIN_NAMESPACE
namespace Ui { class serveurcrawler; }
QT_END_NAMESPACE

class serveurcrawler : public QWidget
{
    Q_OBJECT

public:
    serveurcrawler(QWidget *parent = nullptr);
    ~serveurcrawler();
    double CalculatriceDistance(QPoint pos);
    QPoint DonnerPositionUnique();
    void EnvoyerDonnees(QTcpSocket *client,QPoint pt, QString msg);

    void ViderGrille();
private slots:
    void on_pushButtonLancement_clicked();

    void onQTcpSocket_newConnection();
    void onQTcpSocket_disconnected();
    void onQTcpSocket_readyRead();
    void onQTcpSocket_errorOccured(QAbstractSocket::SocketError socketError);

private:
    Ui::serveurcrawler *ui;
    QPoint tresor;
    QGridLayout *grille;
    QTcpServer socketEcoute;
    QList <QTcpSocket *>listeSocketClient;
    QList <QPoint>listePositions;
    void AfficherGrille();
};
#endif // SERVEURCRAWLER_H
