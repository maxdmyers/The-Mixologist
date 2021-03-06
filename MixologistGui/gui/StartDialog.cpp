/****************************************************************
 *  Copyright 2010, Fair Use, Inc.
 *  Copyright 2006-2007, crypton
 *
 *  This file is part of the Mixologist.
 *
 *  The Mixologist is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  The Mixologist is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the Mixologist; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <interface/init.h>
#include <interface/iface.h>
#include <interface/peers.h>
#include <gui/Util/Helpers.h> //for rot13
#include <gui/Util/GuiSettingsUtil.h>
#include <gui/Util/OSHelpers.h>
#include <gui/StartDialog.h>
#include <gui/WelcomeWizard.h>
#include <version.h>
#include <QFile>
#include <QDesktopServices>
#include <QUrl>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <iostream>


//Setting up global extern for iface.h
LibraryMixerConnect *librarymixerconnect = NULL;

//Setting up global extern for settings.h
QString *startupSettings;
QString *mainSettings;
QString *savedTransfers;

StartDialog::StartDialog(bool* startMinimized, QWidget *parent, Qt::WFlags flags)
    : QMainWindow(parent, flags), startMinimized(startMinimized) {
    loadedOk = false;
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    startupSettings = new QString(Init::getBaseDirectory(true).append("startup.ini"));
    GuiSettingsUtil::loadWidgetInformation(this, *startupSettings);

    ui.loadEmail->setFocus();

    connect(ui.loadButton, SIGNAL(clicked()), this, SLOT(checkVersion()));
    connect(ui.loadPassword, SIGNAL(returnPressed()), ui.loadButton, SLOT(click()));
    connect(ui.loadEmail, SIGNAL(returnPressed()), ui.loadButton, SLOT(click()));

    connect(ui.loadEmail, SIGNAL(textChanged(QString)), this, SLOT(edited()));
    connect(ui.loadPassword, SIGNAL(textChanged(QString)), this, SLOT(edited()));

    connect(ui.autoBox, SIGNAL(stateChanged(int)), this, SLOT(autoLogonChecked(int)));

    librarymixerconnect = new LibraryMixerConnect();
    connect(librarymixerconnect, SIGNAL(downloadedVersion(qulonglong,QString, QString)), this, SLOT(downloadedVersion(qulonglong,QString,QString)), Qt::QueuedConnection);
    //QDomElement is not a default registerd type for QT's meta-object system, so we must first manually register it.
    qRegisterMetaType<QDomElement>("QDomElement");
    connect(librarymixerconnect, SIGNAL(downloadedInfo(QString,unsigned int,QString,QString,QString,QString,QString,QString,QString,QString,QString,QDomElement)),
            this, SLOT(downloadedInfo(QString,unsigned int,QString,QString,QString,QString,QString,QString,QString,QString,QString,QDomElement)), Qt::QueuedConnection);
    connect(librarymixerconnect, SIGNAL(uploadedInfo()), this, SLOT(uploadedInfo()), Qt::QueuedConnection);
    connect(librarymixerconnect, SIGNAL(downloadedFriends()), this, SLOT(downloadedFriends()), Qt::QueuedConnection);
    connect(librarymixerconnect, SIGNAL(downloadedFriendsLibrary()), this, SLOT(finishLoading()), Qt::QueuedConnection);
    connect(librarymixerconnect, SIGNAL(dataReadProgress(int,int)), this, SLOT(updateDataReadProgress(int,int)), Qt::QueuedConnection);
    connect(librarymixerconnect, SIGNAL(errorReceived(int)), this, SLOT(errorReceived(int)), Qt::QueuedConnection);

    ui.progressBar->setVisible(false);
    ui.loadButton->setEnabled(false);

    setWindowTitle("Mixologist Login");

    QSettings settings(*startupSettings, QSettings::IniFormat, this);
    QString email(rot13(settings.value("DefaultEmail", "").toString()));
    QString password(rot13(settings.value("DefaultPassword", "").toString()));
    skip_to_version = settings.value("SkipToVersion", VERSION).toLongLong();
    latest_known_version = skip_to_version;
    if (skip_to_version <= VERSION) {
        skip_to_version = VERSION;
        latest_known_version = VERSION;
        settings.remove("SkipToVersion");
   }

    if (!email.isEmpty()) {
        ui.loadEmail->setText(email);
        if (!password.isEmpty()) {
            ui.loadPassword->setText(password);
        }
    }

    /*
    * Auto-logon has 3 states, which we track with the DefaultPassword setting:
    * No DefaultPassword: Default to auto-login box checked, but obviously we can't login without a password
    * Saved DefaultPassword: Auto-login checked
    * Blank DefaultPassword: Auto-login unchecked
    */

    if (!settings.contains("DefaultPassword") || !settings.value("DefaultPassword").toString().isEmpty()){
        ui.autoBox->setChecked(true);
    }

    //Disable box until fully ready for user interaction
    ui.groupBox->setEnabled(false);
    ui.loadButton->setEnabled(false);

    //All UI elements now setup and ready for display
    if (*startMinimized && settings.contains("DefaultEmail") && settings.contains("DefaultPassword")) showMinimized();
    else show();

    if (!getMixologyLinksAssociated()) {
        if (QMessageBox::question(this,
                                  "Associate Links",
                                  "Mixology links are currently not set up to be opened by the Mixologist.\nSet up now? (Highly recommended)\n\nClicking Mixology links on LibraryMixer will NOT work until they are set up.",
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)
                == QMessageBox::Yes) {
            if (!setMixologyLinksAssociated()) {
                QMessageBox::warning (this,
                                      "Problem writing to registry",
                                      "Unable to set Mixology links to open with the Mixologist because you do not have administrative rights.\n\nStart the Mixologist by right clicking and choosing \"Run as administrator\" to set up the links.",
                                      QMessageBox::Ok);
                exit(1);
            }

        }

    }

    //If we are autologging in, then do so
    if (!ui.loadEmail->text().isEmpty() &&
        !ui.loadPassword->text().isEmpty()) {
        checkVersion();
    } else {
        ui.groupBox->setEnabled(true);
        ui.loadButton->setEnabled(true);
        if (!email.isEmpty()) {
            if (password.isEmpty()) {
                ui.loadPassword->setFocus();
            }
        }
    }
}

void StartDialog::autoLogonChecked(int state) {
    if (state == Qt::Unchecked) {
        QSettings settings(*startupSettings, QSettings::IniFormat, this);
        settings.setValue("DefaultPassword", rot13(""));
    }
}

void StartDialog::edited() {
    if (ui.loadEmail->text() != "" && ui.loadPassword->text() != "") {
        ui.loadButton->setEnabled(true);
    } else {
        ui.loadButton->setEnabled(false);
    }
}

void StartDialog::checkVersion() {
    ui.groupBox->setEnabled(false);
    ui.loadButton->setEnabled(false);

    ui.loadStatus->setText("Connecting");
    ui.progressBar->setVisible(true);
    librarymixerconnect->downloadVersion(skip_to_version);
}

void StartDialog::downloadedVersion(qulonglong _version, QString description, QString importance) {
    if (skip_to_version < _version) {
        latest_known_version = _version;

        QMessageBox msgBox;
        msgBox.setWindowTitle("The Mixologist - Good news!");
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("A new version of the the Mixologist is available. Download now?");
        QPushButton* Yes = msgBox.addButton(QMessageBox::Yes);
        QPushButton* No = msgBox.addButton(QMessageBox::No);
        QPushButton* Never = NULL;
        if (importance == "Essential"){
            msgBox.setInformativeText("This is an essential update.\nYou will not be able to connect until you have upgraded.");
        } else {
            Never = msgBox.addButton("Skip Version", QMessageBox::NoRole);
        }
        msgBox.setDefaultButton(Yes);
        QString details("Changes made since your version:\n");
        msgBox.setDetailedText(details.append(description));
        msgBox.exec();
        if (msgBox.clickedButton() == Yes) {
#if defined(Q_OS_MAC)
            QDesktopServices::openUrl(QUrl(QString("http://www.librarymixer.com/download/mixologist/mac")));
#elif defined(Q_WS_WIN)
            QDesktopServices::openUrl(QUrl(QString("http://www.librarymixer.com/download/mixologist/windows")));
#elif defined(Q_WS_X11)
            QDesktopServices::openUrl(QUrl(QString("http://www.librarymixer.com/download/mixologist/linux")));
#else
            QDesktopServices::openUrl(QUrl(QString("http://www.librarymixer.com/info/mixologist/")));
#endif
            exit(1);
        } else if (msgBox.clickedButton() == No && importance == "Essential"){
            exit(1);
        } else if (Never != NULL && msgBox.clickedButton() == Never) {
            QSettings settings(*startupSettings, QSettings::IniFormat, this);
            settings.setValue("SkipToVersion", _version);
        }

    }

    QString email = ui.loadEmail->text();
    QString password = ui.loadPassword->text();
    librarymixerconnect->setLogin(email, password);

    ui.loadStatus->setText("Downloading User Info");
    ui.progressBar->setValue(20);
    librarymixerconnect->downloadInfo();
}

void StartDialog::downloadedInfo(QString name, unsigned int librarymixer_id,
                                 QString checkout_link1, QString contact_link1, QString link_title1,
                                 QString checkout_link2, QString contact_link2, QString link_title2,
                                 QString checkout_link3, QString contact_link3, QString link_title3,
                                 QDomElement libraryNode) {
    //First time we have the librarymixer_id and can initialize user directory
    Init::loadUserDir(librarymixer_id);
    //First time we have verified that the credentials are good, and can save them to the autoload
    QSettings settings(*startupSettings, QSettings::IniFormat, this);
    settings.setValue("DefaultEmail", rot13(ui.loadEmail->text()));
    if (ui.autoBox->isChecked()){
        settings.setValue("DefaultPassword", rot13(ui.loadPassword->text()));
    } else {
        settings.setValue("DefaultPassword", "");
    }

    //Now that user dir, we can setup the other settings object
    mainSettings = new QString(Init::getUserDirectory(true).append("settings.ini"));
    savedTransfers = new QString(Init::getUserDirectory(true).append("transfers.ini"));

    //Check communications links and make sure at least one is set to librarymixer. If not, prompt to set.
    int link_to_set = -1;
    if ((checkout_link1 != MIXOLOGY_CHECKOUT_LINK || contact_link1 != MIXOLOGY_CONTACT_LINK) &&
        (checkout_link2 != MIXOLOGY_CHECKOUT_LINK || contact_link2 != MIXOLOGY_CONTACT_LINK) &&
        (checkout_link3 != MIXOLOGY_CHECKOUT_LINK || contact_link3 != MIXOLOGY_CONTACT_LINK)) {
        if (checkout_link1 == "") link_to_set = 1;
        else if (checkout_link2 == "") link_to_set = 2;
        else if (checkout_link3 == "") link_to_set = 3;
        else {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Account Setup");
            msgBox.setIcon(QMessageBox::Question);
            if (link_title1 == STANDARD_LINK_TITLE) link_title1 = "Message";
            if (link_title2 == STANDARD_LINK_TITLE) link_title2 = "Message";
            if (link_title3 == STANDARD_LINK_TITLE) link_title3 = "Message";
            msgBox.setText("All 3 of your communications methods are already in use on your LibraryMixer account. Choose one to replace:\n" +
                           link_title1 +
                           "\n" +
                           link_title2 +
                           "\n" +
                           link_title3);
            QAbstractButton *button1 = msgBox.addButton(link_title1, QMessageBox::AcceptRole);
            QAbstractButton *button2 = msgBox.addButton(link_title2, QMessageBox::AcceptRole);
            QAbstractButton *button3 = msgBox.addButton(link_title3, QMessageBox::AcceptRole);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.exec();
            if (msgBox.clickedButton() == button1) link_to_set = 1;
            else if (msgBox.clickedButton() == button2) link_to_set = 2;
            else if (msgBox.clickedButton() == button3) link_to_set = 3;
        }
    }

    ui.loadStatus->setText("Initializing Encryption");
    ui.progressBar->setValue(30);

    /* We can't do the tutorial much earlier because we rely upon the mainSettings variable to be initialized.
       We can't do the tutorial much later because we need to know at CreateControl whether off-LibraryMixer is enabled. */
    QSettings userSettings(*mainSettings, QSettings::IniFormat, this);
    if (!userSettings.value("Tutorial/Initial", DEFAULT_TUTORIAL_DONE_INITIAL).toBool()) {
        WelcomeWizard wiz;
        wiz.exec();
    }

    QString public_key = Init::InitEncryption(librarymixer_id);
    if (public_key.isEmpty()) {
        QMessageBox::warning (this,
                              "Something has gone wrong",
                              "Unable to start encryption",
                              QMessageBox::Ok);
        exit(1);
    }

    //Must start server now so that we can get the IP address to upload in the next step
    Init::createControl(name);
    Init::loadLibrary(libraryNode);

    ui.loadStatus->setText("Updating Info");
    ui.progressBar->setValue(40);

    librarymixerconnect->uploadInfo(link_to_set, public_key);
}

void StartDialog::uploadedInfo() {
    ui.loadStatus->setText("Downloading Friend List");
    ui.progressBar->setValue(60);

    librarymixerconnect->downloadFriends();
}

void StartDialog::downloadedFriends() {
    ui.loadStatus->setText("Downloading Mixed Library");
    ui.progressBar->setValue(80);

    librarymixerconnect->downloadFriendsLibrary();
}

void StartDialog::finishLoading() {
    control->setVersion("Mixologist", VERSION, latest_known_version);
    control->StartupMixologist();
    loadedOk = true;
    close();
}

void StartDialog::updateDataReadProgress(int bytesRead, int totalBytes) {
    if (totalBytes != 0){
        if (ui.progressBar->value() < 20) { //Step 1
            ui.progressBar->setValue(bytesRead/totalBytes);
        } else if (20 <= ui.progressBar->value() && ui.progressBar->value() < 30) { //Step 2.0
            ui.progressBar->setValue(20 + (bytesRead/totalBytes));
        } else if (30 <= ui.progressBar->value() && ui.progressBar->value() < 50) { //Step 2.5
            ui.progressBar->setValue(30 + (bytesRead/totalBytes));
        } else if (50 <= ui.progressBar->value() && ui.progressBar->value() < 60) { //Step 3
            ui.progressBar->setValue(50 + (bytesRead/totalBytes));
        } else if (60 <= ui.progressBar->value() && ui.progressBar->value() < 80) { //Step 4
            ui.progressBar->setValue(60 + (bytesRead/totalBytes));
        } else if (80 <= ui.progressBar->value() && ui.progressBar->value() < 100) { //Step 5
            ui.progressBar->setValue(80 + (bytesRead/totalBytes));
        }
    }
}

void StartDialog::errorReceived(int errorCode) {
    if (errorCode == LibraryMixerConnect::version_download_error) {
        ui.loadStatus->setText("Unable to connect");
    } else if (errorCode == LibraryMixerConnect::ssl_error) {
        ui.loadStatus->setText("Unable to connect");
    } else if (errorCode == LibraryMixerConnect::bad_login_error || errorCode == LibraryMixerConnect::info_download_error) {
        ui.loadStatus->setText("There was a problem\nwith your login");
    } else {
        QMessageBox::warning (this,
                              "The Mixologist",
                              "Ran into a problem while starting up, give it another try (" + QString::number(errorCode) + ").",
                              QMessageBox::Ok);
        exit(1);
    }

    ui.groupBox->setEnabled(true);
    ui.loadButton->setEnabled(true);

    ui.progressBar->setValue(0);
    ui.progressBar->setVisible(false);

    this->show();
}

void StartDialog::closeEvent (QCloseEvent *event) {
    {
        QSettings settings(*startupSettings, QSettings::IniFormat, this);
        settings.setValue("Version", VERSION);
        GuiSettingsUtil::saveWidgetInformation(this, *startupSettings);
    }//flush the QSettings object so it saves

    if (loadedOk) {
        *startMinimized = !isMinimized();
        this->deleteLater();
        QWidget::closeEvent(event);
    } else {
        exit(1);
    }
}
