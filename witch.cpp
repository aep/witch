#include "error.h"
#include "modules.h"

extern "C" {
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
}

#include <typeinfo>

#include <QByteArray>
#include <QList>
#include <QFile>
#include <QDir>

#include <QDebug>

int main(int argc, char **argv)
{
    FILE *tty;
    try {
        tty = fopen("/dev/tty", "a+");
        if (!tty) throw PosixException("tty");

        Grant g;
        g.constraints = 0;

        if (!isatty(fileno(stdin))) throw PolicyException("stdin is not a typewriter! bad robot!");
        if (getuid() != 1000) throw PolicyException("uid not human");
        if (seteuid(0) != 0) throw PosixException("seteuid");
        if (setuid(0) != 0) throw PosixException("setuid");
        if (setgid(0) != 0) throw PosixException("setgid");

        // find program to execute

        g.program = argv[1];
        if (g.program.isEmpty()) throw std::invalid_argument("expecting arguments");

        if (g.program.at(0) != '/') {
            if (QDir::current().exists(g.program))
                g.program = QDir::current().absoluteFilePath(g.program);

            QList<QByteArray> pathL = QByteArray(getenv("PATH")).split(':');
            foreach (const QByteArray &path, pathL) {
                struct stat st;
                if (stat(qPrintable(path + '/' + g.program), &st) == 0) {
                    g.program = path + '/' + g.program;
                    break;
                }
            }
        }
        if (g.program.at(0) != '/') throw PolicyException("not in PATH");

        g.program = QDir::cleanPath(g.program);

        for (int a = 2; a < argc; a++) {
            g.arguments.append(QString::fromLocal8Bit(argv[a]));
        }

        // gather information about executor

        g.ppid = getppid();
        QFile pcmdline("/proc/" + QString::number(g.ppid) + "/cmdline");
        if (!pcmdline.open(QFile::ReadOnly)) throw PosixException("/proc/ppid/cmdline");
        QList<QByteArray> pcmd =  pcmdline.readAll().split(0);
        pcmd.takeFirst();
        foreach(QByteArray parg, pcmd)
            g.parentargs.append(QString::fromLocal8Bit(parg));

        char buff[2048];
        buff[readlink(qPrintable("/proc/" + QString::number(g.ppid) + "/exe"), buff, 2048)] = 0;
        g.parent = QString::fromLocal8Bit(buff);
        buff[readlink(qPrintable("/proc/" + QString::number(g.ppid) + "/cwd"), buff, 2048)] = 0;
        g.pcwd = QString::fromLocal8Bit(buff);


        // now ask user for grant to perform this
        int e = 0;
        e = grant_tty(&g);
        if (e < 0)
            exit (e);

        setenv("HOME", "/root", 1);
        setenv("USER", "root", 1);

    } catch (std::exception &e) {
        FILE *f = tty;
        if (!f)
            f = stderr;
        fprintf(f, "nuwitch: %s: %s\n", typeid(e).name(), e.what());
        exit (4);
    }

    execvp(argv[1], argv + 1);
    perror(argv[0]);
    return errno;
}
