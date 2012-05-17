#include <QStringList>

enum GrantResult
{
    GrantAgain   = -2,
    GrantFail    = -1,
    GrantPass    = 0,
    GrantSkip    = 1,
};

enum GrantConstraints
{
    GrantAll   = 0,
    GrantTrace = 1,
    GrantIsolate = 2
};

struct Grant
{
    QString program;
    QStringList arguments;

    QString parent;
    QStringList parentargs;
    QString pcwd;
    int ppid;

    int constraints;
};

int grant_tty(Grant *);
