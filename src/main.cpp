#include <QCoreApplication>
#include <QProcess>
#include <QDebug>
#include <QFile>
#include <QThread>

#include <iostream>

QProcess proc;
QFile outputFile("output.txt");

void output(const QString& msg, bool newline = true)
{
    outputFile.write(msg.toUtf8());
    if(newline)
        outputFile.write("\n");
}

// parent -> hijacker -> target
void toTarget()
{
    QByteArray procBuf;
    while(proc.canReadLine())
    {
        int len;
        procBuf = proc.readLine();
        len = procBuf.size();
        std::cout.write(procBuf.data(), len);
        if(len > 1 && procBuf[len - 2] == '\r' && procBuf[len - 1] == '\n') // output single line
            procBuf.remove(len - 2, 1);
        output("<< " + procBuf, false);
        fflush(stdout);
    }
    QThread::msleep(10);
    QCoreApplication::processEvents(); // necessary
}

// parent <- hijacker <- target
// getline() will block.
void toParent()
{
    std::string inBuf;
    std::getline(std::cin, inBuf);
    inBuf.append("\n");
    proc.write(inBuf.data(), inBuf.size());
    output(">> " + QString::fromStdString(inBuf), false);

}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QFile* configFile = new QFile("hijack_config.txt");

    outputFile.open(QFile::WriteOnly | QFile::Unbuffered | QFile::Text);

    // read the first line as the target filename
    output("Opening " + configFile->fileName());
    configFile->open(QFile::ReadOnly);

    proc.setProgram(configFile->readLine());
    output("Target: " + proc.program());

    // pass the arguments to the target process
    QStringList argList;
    QString argStr;
    argList = a.arguments();
    argList.removeFirst();
    proc.setArguments(argList);
    for(auto it = argList.begin(); it != argList.end(); it++)
        argStr += "\"" + *it + "\" ";
    output("Arguments: " + argStr);

    // start target process
    output("Starting...");
    proc.start();
    if(!proc.waitForStarted(10000) || proc.state() != QProcess::Running)
    {
        output("Failed to start!");
        return a.exec();
    }

    output("Started");

    // hijack
    while(1)
    {
        for(int i = 0; i < 600; i++)
            toTarget();
        toParent();
    }

    outputFile.close();
    return a.exec();
}
