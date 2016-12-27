#include <stdio.h>     /* printf()、fprintf()に必要 */
#include <sys/socket.h> /* socket()、bind()、connect()に必要 */
#include <arpa/inet.h>  /* sockaddr_in、inet_ntoa()に必要 */
#include <stdlib.h>    /* atoi()に必要 */
#include <string.h>    /* memset()に必要 */
#include <unistd.h>    /* close()に必要 */
#include <fcntl.h>     /* fcntl()に必要 */
#include <sys/file.h>  /* O_NONBLOCK、FASYNCに必要 */
#include <signal.h>     /* signal()、SIGALRIAに必要 */
#include <errno.h>     /* errnoに必要 */

#define ECHOMAX 255    /* エコー文字列の最大長 */

void DieWithError(char *errorMessage); /* エラー処理関数 */
void UseIdleTime();                    /* アイドル時間を利用する関数 */
void SIGIOHandler(int signalType);     /* SIGIOのハンドラ関数 */

int sock;                        /* ソケット（シグナルハンドラ用にグローバルに作成） */

int main(int argc, char *argv[])
{
    struct sockaddr_in echoServAddr;  /* サーバのアドレス */
    unsigned short echoServPort;      /* サーバのポート番号 */
    struct sigaction handler;         /* シグナル処理のアクションを定義 */

   /* パラメータの数が正しいか確認 */
   if (argc != 2)
   {
       fprintf(stderr,"Usage: %s <SERVER PORT>\n", argv[0]);
       exit(1);
   }

   echoServPort = atoi(argv[1]); /* 1つ目の引数： ローカルポート番号 */

   /* データグラムの送受信に使うソケットを作成 */
   if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
      DieWithError("socket() failed");

   /* サーバのアドレス構造体をセットアップ */
   memset(&echoServAddr, 0, sizeof(echoServAddr));    /* 構造体にゼロを埋める */
   echoServAddr.sin_family = AF_INET;                 /* インターネットファミリ */
   echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* 任意の受信インタフェース */
   echoServAddr.sin_port = htons(echoServPort); /* ポート */

   /* ローカルアドレスへバインド */
   if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
       DieWithError("bind() failed");

   /* SIGIOのシグナルハンドラを設定 */
   handler.sa_handler = SIGIOHandler;
   /* 全シグナルをマスクするマスクを作成 */
   if (sigfillset(&handler.sa_mask) < 0)
     DieWithError("sigfillset() failed");
   /* フラグなし */
   handler.sa_flags = 0;

   if (sigaction(SIGIO, &handler, 0) < 0)
       DieWithError("sigaction() failed for SIGIO");

   /* ソケットを所有してSIGIOメッセージを受信させる */
   if (fcntl(sock, F_SETOWN, getpid()) < 0)
     DieWithError("Unable to set process owner to us");

   /* ノンブロッキングI/OとSIGIO送信をセットアップ */
   if (fcntl(sock, F_SETFL, O_NONBLOCK | FASYNC) < 0)
       DieWithError("Unable to put client sock into nonblocking/async mode");

   /* 処理を離れてほかの仕事をする（エコーはバックグラウンドで発生する） */

   for (;;)
       UseIdleTime();

   /* この部分には到達しない */
}

void UseIdleTime()
{
    printf(".\n");
    sleep(3);     /* 3秒間動作する */
}

void SIGIOHandler(int signalType)
{
    struct sockaddr_in echoClntAddr; /* データグラムの送信元アドレス */
    unsigned int clntLen;            /* アドレス長 */
    int recvMsgSize;                /* データグラムのサイズ */
    char echoBuffer[ECHOMAX];       /* データグラムのバッファ */

    do /* 入力データがなくなるまで実行 */
    {
       /* 入出力パラメータのサイズを設定 */
       clntLen = sizeof(echoClntAddr);

       if ((recvMsgSize = recvfrom(sock, echoBuffer, ECHOMAX, 0,
              (struct sockaddr *) &echoClntAddr, &clntLen)) < 0)
       {
           /* recvfrom()がブロックしようとした場合（許容される唯一のエラー） */
           if (errno != EWOULDBLOCK)
               DieWithError("recvfrom() failed");
       }
       else
       {
           printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

           if (sendto(sock, echoBuffer, recvMsgSize, 0, (struct sockaddr *)
                 &echoClntAddr, sizeof(echoClntAddr)) != recvMsgSize)
               DieWithError("sendto() failed");
        }
     } while (recvMsgSize >= 0);
     /* すべての受信が完了 */
 }
