#include "serveurcrawler.h"
#include "ui_serveurcrawler.h"
#include <QRandomGenerator>
#include <QDateTime>
#include <QThread>
#include <QBuffer>

serveurcrawler::serveurcrawler(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::serveurcrawler)
{
    ui->setupUi(this);
    grille=new QGridLayout(this);



    for(int ligne=0; ligne<TAILLE; ligne++)
    {
        for (int colonne=0; colonne<TAILLE; colonne++)
        {
            QToolButton *b=new QToolButton();
            grille->addWidget(b,ligne,colonne,1,1);
        }
    }
    grille->addWidget(ui->labelPort,TAILLE,1,2,1);
    grille->addWidget(ui->spinBoxPort,TAILLE,3,2,1);
    grille->addWidget(ui->pushButtonLancement,TAILLE,5,2,1);
    grille->addWidget(ui->pushButtonQuitter,TAILLE,7,2,1);

    this->setLayout(grille);

    connect(&socketEcoute,&QTcpServer::newConnection,this,&::serveurcrawler::onQTcpSocket_newConnection);

}

serveurcrawler::~serveurcrawler()
{
    delete ui;
}

double serveurcrawler::CalculatriceDistance(QPoint pos)
{
    double distance;
    int xVecteur = tresor.x()-pos.x();
    int yVecteur = tresor.y()-pos.y();
    distance = sqrt((xVecteur*xVecteur + yVecteur*yVecteur));
    return distance;
}

QPoint serveurcrawler::DonnerPositionUnique()
{
    QRandomGenerator gen;
    QPoint pt;
    gen.seed(QDateTime::currentMSecsSinceEpoch());
    int ligne;
    int colonne;
    do
    {
        ligne = gen.bounded(TAILLE);
        QThread::usleep(20000);	// attendre 20ms
        colonne = gen.bounded(TAILLE);
        QThread::usleep(20000);	// attendre 20ms
        pt = QPoint(colonne,ligne);
    }while (listePositions.contains(pt));
    return pt;
}

void serveurcrawler::EnvoyerDonnees(QTcpSocket *client, QPoint pt, QString msg)
{
    quint16 taille = 0;
    QBuffer tampon;
    double distance=CalculatriceDistance(pt);
    tampon.open(QIODevice::WriteOnly);

    QDataStream out(&tampon);

    out<<taille<<pt<<msg,distance;

    taille =static_cast<quint16>(tampon.size())-sizeof(taille);

    tampon.seek(0);
    out<<taille;
    client->write(tampon.buffer());
}


void serveurcrawler::on_pushButtonLancement_clicked()
{
    if(socketEcoute.listen(QHostAddress::Any,ui->spinBoxPort->value())){
        qDebug()<<"lancement ok";
        tresor=DonnerPositionUnique();
    }else{
        qDebug()<<socketEcoute.errorString();
    }
}

void serveurcrawler::onQTcpSocket_newConnection()
{
    // récupération de la socket de communication du client
    QTcpSocket *client=socketEcoute.nextPendingConnection();

    // connection signal/slot pour la socket
    connect(client,&QTcpSocket::disconnected,this,&serveurcrawler::onQTcpSocket_disconnected);
    connect(client,&QTcpSocket::readyRead,this,&serveurcrawler::onQTcpSocket_readyRead);
    connect(client,&QTcpSocket::errorOccurred,this,&serveurcrawler::onQTcpSocket_errorOccured);
    // création et ajout du client dans la liste des clients
    listeSocketClient.append(client);
    QPoint pos=DonnerPositionUnique();
    listePositions.append(pos);
    // envoyer la listes des vols au client entrant
    EnvoyerDonnees(client,pos,"start");
    AfficherGrille();

}

void serveurcrawler::onQTcpSocket_disconnected()
{
    QTcpSocket *client=qobject_cast<QTcpSocket *>(sender());
    int indexClient = listeSocketClient.indexOf(client);
    //retirer le client de la liste des clients
    listeSocketClient.removeOne(client);
    //retirer la position
    listePositions.removeAt(indexClient);

}

void serveurcrawler::onQTcpSocket_readyRead()
{
    qDebug() << "Ready to read.";
    quint16 taille = 0;
    QChar commande;
    int ref;
    int indexClient;
    QTcpSocket *client=qobject_cast<QTcpSocket *>(sender());
    indexClient = listeSocketClient.indexOf(client);
    QPoint positionClient = listePositions.at(indexClient);

    if(client->bytesAvailable() >= (quint16)sizeof(taille)){
        QDataStream in(client);
        in >> taille;
        if(client->bytesAvailable() >= (quint64)taille){
            in>>commande;
            switch (commande.toLatin1()){
            case 'U':
                positionClient.setY(positionClient.y()-1);
                break;

            case 'D':
                positionClient.setY(positionClient.y()+1);
                break;

            case 'L':
                positionClient.setX(positionClient.x()-1);
                break;

            case 'R':
                positionClient.setX(positionClient.x()+1);
                break;
            }
            //Collision ?
            if(listePositions.contains(positionClient)){
                int indexAutre=listePositions.indexOf(positionClient);
                QPoint posAutre = DonnerPositionUnique();
                QPoint posClient = DonnerPositionUnique();
                listePositions.replace(indexAutre, posAutre);
                listePositions.replace(indexClient, posClient);
                EnvoyerDonnees(client, posClient, "collision");
                EnvoyerDonnees(listeSocketClient.at(indexAutre), posAutre, "collision");
            }
            else{ //Pas  de Collision
                if(positionClient == tresor){
                    foreach(QTcpSocket *client, listeSocketClient){
                        EnvoyerDonnees(client, QPoint(-1, -1), "Victoire de"+client->peerAddress().toString());
                    }
                    //Deconnection
                    foreach(QTcpSocket *client, listeSocketClient){
                        client->disconnectFromHost();
                    }
                    tresor=DonnerPositionUnique();
                }
                else{
                    //cas des bords
                    if(positionClient.x()<0){
                        positionClient.setX(1);
                    }
                    if(positionClient.x()>TAILLE-1){
                        positionClient.setX(TAILLE-2);
                    }
                    if(positionClient.y()<0){
                        positionClient.setY(1);
                    }
                    if(positionClient.y()>(TAILLE-1)){
                        positionClient.setY(TAILLE-2);
                    }
                    listePositions.replace(indexClient, positionClient);
                    EnvoyerDonnees(client, positionClient, "vide");



                }

            }

        }
        AfficherGrille();
    }
}

void serveurcrawler::onQTcpSocket_errorOccured(QAbstractSocket::SocketError socketError)
{

    QTcpSocket *client=qobject_cast<QTcpSocket *>(sender());
    qDebug()<<client->errorString();

}

void serveurcrawler::AfficherGrille()
{
    ViderGrille();
    //afficher les joueur
    foreach(QPoint pt,listePositions){
        grille->itemAtPosition(pt.y(),pt.x())->widget()->setStyleSheet("background-color: black");
    }
    //afficher le tresor
    grille->itemAtPosition(tresor.y(),tresor.x())->widget()->setStyleSheet("background-color: red");

}

void serveurcrawler::ViderGrille(){
    for(int ligne=0;ligne<TAILLE;ligne++){
        for(int colonne=0;colonne<TAILLE;colonne++){
            grille->itemAtPosition(ligne,colonne)->widget()->setStyleSheet("background-color : white");

        }
    }
}

