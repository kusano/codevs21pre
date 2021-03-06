#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <vector>
#include <string>
#include <algorithm>
using namespace std;

#define     TEMP        template<int W, int H, int T, int S, int N, int P, int Th>
#define     PARAM       W,H,T,S,N,P,Th



//  操作
struct MOVE
{
    int         X, R;
                MOVE() {}
                MOVE( int x, int r ): X(x), R(r) {}
    
    bool operator<(const MOVE &m) const { return false; }
};

TEMP struct STATE
{
    char        F[W*H+1];           //  フィールド
    int         Fc;                 //  発火数カウント
    int         step;               //  現在のステップ数
    long long   score;              //  現在のスコア
    int         blocknum;           //  現在のブロック数
    int         height[W];          //  各列の高さ
};

//  盤面
//  コンパイル後に各パラメタが定数になったほうが高速（かもしれない）ので、
//  クラステンプレートを用いて実装
template<
    int         W,                  //  幅
    int         H,                  //  高さ（入力のH+T）
    int         T,                  //  パックの一辺の長さ
    int         S,                  //  消える値
    int         N,                  //  パック数
    int         P,                  //  点数のパラメタ
    int         Th>                 //  点数のパラメタ
class Field 
{
    const static long long INF = 1LL<<60;   //  無限大
    const static int    pspin[4][T*T];      //  パック回転
    const static int    linenum = 3*(W+H)-2;//  フィールド中の直線数
    const static int    line[linenum][4];   //  フィールド中の直線
    const static int    pos2line[W*H][4];   //  ブロックが含まれる直線
    const static int    neighbor8[W*H][8];  //  8近傍

    char        pack[N][T*T];       //  パック

    char        F[W*H+1];           //  フィールド
    int         Fc;                 //  発火数カウント
    int         step;               //  現在のステップ数
    long long   score;              //  現在のスコア
    int         blocknum;           //  現在のブロック数
    int         height[W];          //  各列の高さ
    
    int         pleft[N][4];        //  最左のパック落下位置
    int         pright[N][4];       //  最右のパック落下位置

    char        hist_F[N][W*H];     //  フィールド履歴
    int         hist_Fc[N];         //  発火数カウント履歴
    long long   hist_score[N];      //  スコア履歴
    int         hist_blocknum[N];   //  ブロック数履歴
    int         hist_height[N][W];  //  高さ履歴

    void        move( const char p[T*T], int xpos );
public:
                Field( vector<vector<vector<int>>> p );
    bool        check( MOVE m ) const;
    void        move( MOVE m );
    void        undo();
    STATE<PARAM>getstate() const;
    void        setstate( const STATE<PARAM> &s );
    int         getFc() const { return Fc; }
    int         getstep() const { return step; }
    long long   getscore() const { return score; }
    void        getideal( long long *score=NULL, int *bpos=NULL, int *bval=NULL );
    long long   getidealscore() { long long s; getideal(&s); return s; }
    int         getblocknum() const { return blocknum; }
    string      tostring();
};

    