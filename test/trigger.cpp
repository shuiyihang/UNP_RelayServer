#include "generator.h"

int main(int argc,char* argv[])
{
    int session = 5;// 建立的会话数
    int talklen = 50;// 会话长度
    if(argc == 3)
    {
        session = atoi(argv[1]);
        talklen = atoi(argv[2]);
    }else if(argc == 2)
    {
        session = atoi(argv[1]);
    }
    printf("session:%d,talklen:%d\n",session,talklen);
    CLGenerator* trigger = new CLGenerator;
    trigger->init(session,talklen);

    trigger->build_contact();

    trigger->run();
}