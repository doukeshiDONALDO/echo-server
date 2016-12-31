#include "TCPEchoServer.h" /* TCPエコーサーバのヘッダファイルをインクルード */
#include <pthread.h>      /* POSIXスレッドに必要 */
#include <sys/socket.h> /* socket()、bind()、connect()に必要 */
#include <arpa/inet.h>  /* sockaddr_in and inet_ntoa()に必要 */
#include <string.h>     /* memset()に必要 */
#include <stdio.h>  /* printf()、fprintf()に必要 */
#include <unistd.h> /* close()に必要 */


#define MAXPENDING 5    /* 未処理の接続要求の最大値 */
#define RCVBUFSIZE 32 /* 受信バッファのサイズ */

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

void *ThreadMain(void *arg);          /* スレッドのメインプログラム */

/* クライアントスレッドに渡す引数の構造体 */
struct ThreadArgs
{
int clntSock;                          /* クライアントのソケットディスクリプタ */
};


int AcceptTCPConnection(int servSock)
{
    int clntSock;                   /* クライアントのソケットディスクリプタ */
    struct sockaddr_in echoClntAddr; /* クライアントのアドレス */
    unsigned int clntLen;           /* クライアントのアドレス構造体の長さ */

    /* 入出力パラメータのサイズをセット */
    clntLen = sizeof(echoClntAddr);

    /* クライアントからの接続要求を待つ */
    if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr,
           &clntLen)) < 0)
        DieWithError("accept() failed");

     /* clntSockはクライアントに接続済み */

     printf ("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

     return clntSock;
}

void HandleTCPClient(int clntSocket)
{
  char echoBuffer[RCVBUFSIZE];  /* エコー文字列のバッファ */
  int recvMsgSize;  /* 受信メッセージのサイズ */

  /* クライアントからの受信メッセージ */
  if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
    DieWithError("recv() failed");

  /* 受信した文字列を送信し、転送が終了していなければ次を受信する */
  while (recvMsgSize > 0) /* ゼロは転送の終了を意味する */
  {
    printf(echoBuffer);
    /* メッセージをクライアントにエコーバックする */
    if (send(clntSocket, echoBuffer, recvMsgSize, 0) != recvMsgSize)
      DieWithError("send() failed");

    /* 受信するデータが残っていないか確認する */
    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
      DieWithError("recv() failed");
  }

  close(clntSocket);  /* クライアントソケットをクローズする */
}

int main(int argc, char *argv[])
{
    int servSock;                    /* サーバのソケットディスクリプタ */
    int clntSock;                    /* クライアントのソケットディスクリプタ */
    unsigned short echoServPort;     /* サーバのポート */
    pthread_t threadID;              /* pthread_create()が返すスレッドID */
    struct ThreadArgs *threadArgs;   /* スレッドの引数構造体へのポインタ */

    if (argc != 2)    /* 引数の数が正しいか確認 */
    {
        fprintf(stderr,"Usage: %s <SERVER PORT>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]); /* 1つ目の引数： ローカルポート */


    int sock;                        /* 作成するソケット */
    struct sockaddr_in echoServAddr; /* ローカルアドレス */

    /* 着信接続要求に対するソケットを作成 */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* ローカルのアドレス構造体を作成 */
    memset(&echoServAddr, 0, sizeof(echoServAddr));    /* 構造体をゼロで埋める */
    echoServAddr.sin_family = AF_INET;                /* インターネットアドレスファミリ */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* ワイルドカードを使用 */
    echoServAddr.sin_port = htons(echoServPort);              /* ローカルポート */

    /* ローカルアドレスへバインド */
    if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");

    /* 着信接続要求のリスン中というマークをソケットに付ける */
    if (listen(sock, MAXPENDING) < 0)
        DieWithError("listen() failed");

    servSock = sock;

    for (;;) /* プログラムが終了するまで繰り返し実行 */
    {
        clntSock = AcceptTCPConnection(servSock);

        /* クライアント引数用にメモリを新しく確保 */
        if ((threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs)))
               == NULL)
             DieWithError("malloc() failed");
        threadArgs -> clntSock = clntSock;

        /* クライアントスレッドを生成 */
        if (pthread_create(&threadID, NULL, ThreadMain, (void *) threadArgs) != 0)
            DieWithError("pthread_create() failed");
        printf("with thread %ld\n", (long int) threadID);
    }
    /* この部分には到達しない */
}

void *ThreadMain(void *threadArgs)
{
    int clntSock; /* クライアント接続用ソケットディスクリプタ */

    /* 戻り時に、スレッドのリソースを割り当て解除 */
    pthread_detach(pthread_self());

    /* ソケットのファイルディスクリプタを引数から取り出す */
    clntSock = ((struct ThreadArgs *) threadArgs) -> clntSock;
    free(threadArgs);              /* 引数に割り当てられていたメモリを解放 */

    HandleTCPClient(clntSock);

    return (NULL);
}

