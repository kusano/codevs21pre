#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <cstdlib>
#include <utility>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <process.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "field.h"

using namespace std;



//  スレッド数
#define     THREAD_NUM      7

//  FCを増やすフェーズの制限時間
#define     FC_TIMELIMIT    (1000*CLOCKS_PER_SEC)

//  FCフェーズのステップ数
#define     FC_STEP_S       500
#define     FC_STEP_M       500
#define     FC_STEP_L       500

//  FCフェーズのビーム幅
#define     FC_WIDTH_S      1024
#define     FC_WIDTH_M      1024
#define     FC_WIDTH_L      512

//  スコアを増やすフェーズの制限時間
#define     SC_TIMELIMIT    (8000*CLOCKS_PER_SEC)

//  スコアフェーズの1回あたりのステップ数
#define     SC_STEP_S       64
#define     SC_STEP_M       64
#define     SC_STEP_L       64

//  スコアフェーズのビーム幅
#define     SC_WIDTH_S      128
#define     SC_WIDTH_M      64
#define     SC_WIDTH_L      16

//  各サイズのパラメタを取得
#define     SML(P,W)        (W==10 ? P##_S : W==15 ? P##_M : P##_L)



//  ビームサーチ用
TEMP struct NODE
{
    long long   score;              //  スコア
    STATE<PARAM>state;              //  盤面
    int         prev;               //  直前の状態
    MOVE        move;               //  直前の状態からの指手

    NODE() {};
    NODE( long long sc, STATE<PARAM> st, int pr, MOVE mv )
        : score(sc), state(st), prev(pr), move(mv) {}
    bool operator<( const NODE &n ) { return score<n.score; }
};



static int                          W, H, T, S, N;
static CRITICAL_SECTION             lock;
static vector<vector<vector<int>>>  pack;
static vector<MOVE>                 move;
static vector<vector<MOVE>>         mover;
static vector<long long>            score;
static bool                         term;
static vector<bool>                 finish;



TEMP static int main2();
TEMP static void __cdecl threadFc( void *arglist );
TEMP static void solveFc( int id, Field<PARAM> F, vector<MOVE> *move, int *Fc );
TEMP static void __cdecl threadSc( void *arglist );
TEMP static void solveSc( int id, Field<PARAM> F, vector<MOVE> *move, long long *score );
static void checkTerminate( int id );



int main()
{
    //  最初の1行を読み込みテンプレート版へ
    int W, H, T, S, N;
    cin>>W>>H>>T>>S>>N;

    if ( W==10 )  return main2< 10, 16+4, 4, 10, 1000, 25,   100>();
    if ( W==15 )  return main2< 15, 23+4, 4, 20, 1000, 30,  1000>();
    if ( W==20 )  return main2< 20, 36+5, 5, 30, 1000, 35, 10000>();

    return -1;
}

TEMP int main2()
{
    ::pack = vector<vector<vector<int>>>( N, vector<vector<int>>(T,vector<int>(T)) );
    for ( int n=0; n<N; n++ )
    {
        for ( int y=0; y<T; y++ )
        for ( int x=0; x<T; x++ )
            cin>>pack[n][T-y-1][x];
        string end;
        cin>>end;
    }

    //  ログファイル
    string fname = "log\\log_";

    if ( W==10 ) fname += "s_";
    if ( W==15 ) fname += "m_";
    if ( W==20 ) fname += "l_";

    time_t ctm = time(NULL);
    tm ltm;
    localtime_s( &ltm, &ctm );
    char tmp[32];
    strftime( tmp, sizeof tmp, "%Y%m%d_%H%M%S.txt", &ltm );
    fname += tmp;

    ofstream log(fname);

    //  ================
    //  Fcフェーズ
    //  ================

    clock_t start = clock();

    InitializeCriticalSection(&::lock);

    ::mover = vector<vector<MOVE> >();
    ::score = vector<long long>();
    ::term = false;
    ::finish = vector<bool>( THREAD_NUM, false );

    for ( int i=0; i<THREAD_NUM; i++ )
        _beginthread( threadFc<PARAM>, 0, (void *)i );

    while ( true )
    {
        //  制限時間をオーバーしていて、1個でも結果が出ていたら終了
        EnterCriticalSection(&::lock);
        if ( ::mover.size() > 0 &&
             clock()-start > FC_TIMELIMIT )
            ::term = true;
        LeaveCriticalSection(&::lock);

        //  全てのスレッドが終了状態になったら終了
        EnterCriticalSection(&::lock);
        bool f = true;
        for ( int i=0; i<THREAD_NUM; i++ )
            if ( !::finish[i] )
                f = false;
        LeaveCriticalSection(&::lock);
        if ( f )
            break;

        Sleep( 1000 );
    }

    log << "Fc Phase end: " << (double)clock()/CLOCKS_PER_SEC << endl;
    for ( int i=0; i<(int)::score.size(); i++ )
        log << i << " " << score[i] << endl;

    //  最高点の試行を採用
    int best = 0;
    for ( int i=1; i<(int)::score.size(); i++ )
        if ( score[i] > score[best] )
            best = i;

    ::move = ::mover[best];

    log << "Best: " << score[best] << endl;

    //  ================
    //  スコアフェーズ
    //  ================
    
    clock_t gstart = clock();

    for ( int n=SML(FC_STEP,W); n+16<N; )
    {
        //  今回の制限時間
        long long timelimit = (SC_TIMELIMIT-(clock()-gstart)) / ((N-n)/32+4);
        
        clock_t start = clock();

        //  スレッド起動
        ::mover = vector<vector<MOVE> >();
        ::score = vector<long long>();
        ::term = false;
        ::finish = vector<bool>( THREAD_NUM, false );

        for ( int i=0; i<THREAD_NUM; i++ )
            _beginthread( threadSc<PARAM>, 0, (void *)i );

        while ( true )
        {
            //  制限時間をオーバーしていて、1個でも結果が出ていたら終了
            EnterCriticalSection(&::lock);
            if ( ::mover.size()>0 &&
                 clock()-start > timelimit )
                ::term = true;
            LeaveCriticalSection(&::lock);

            //  全てのスレッドが終了状態になったら終了
            EnterCriticalSection(&::lock);
            bool f = true;
            for ( int i=0; i<THREAD_NUM; i++ )
                if ( !::finish[i] )
                    f = false;
            LeaveCriticalSection(&::lock);
            if ( f )
                break;

            Sleep( 1000 );
        }

        log << "Sc Phase end: " << (double)clock()/CLOCKS_PER_SEC << endl;

        //  スコア増加／ステップ数が最高の試行を採用
        int best = -1;
        double bestr = -1e100;

        for ( int i=0; i<(int)::mover.size(); i++ )
        {
            double r = (double)::score[i]/::mover[i].size();

            log << i << " " << ::mover[i].size() << " " <<
                ::score[i] << " " << r << endl;

            if ( r > bestr )
                bestr = r,
                best = i;
        }

        ::move.insert(::move.end(), ::mover[best].begin(), ::mover[best].end());
        n += (int)::mover[best].size();

        log << "Best: " << ::mover[best].size() << " " <<
                ::score[best] << " " << bestr << endl;
        log << "n: " << n << endl;
    }

    //  N手にする
    while ( ::move.size()<N )
        ::move.push_back(MOVE(0,0));

    //  出力
    for ( int i=0; i<(int)::move.size(); i++ )
        cout << ::move[i].X << " " << ::move[i].R << endl;

    //  ログに盤面の様子を出力
    Field<PARAM> F(pack);
    for ( int i=0; i<(int)::move.size(); i++ )
    {
        F.move(::move[i]);
        log << F.tostring() << endl;
    }

    //  実行時間
    log << "Time: " << (double)clock()/CLOCKS_PER_SEC << endl;

    return 0;
}



//  Fcフェーズのスレッドエントリーポイント
//  moverに手順、scoreにスコアを追加していく
TEMP void __cdecl threadFc( void *arglist )
{
    int id = (int)arglist;
    srand(id);
    
    while ( true )
    {
        Field<PARAM> F(pack);
        for ( int i=0; i<(int)::move.size(); i++ )
            F.move(::move[i]);

        vector<MOVE> move;
        int Fc;
        solveFc<PARAM>( id, F, &move, &Fc );

        //  保存
        EnterCriticalSection(&::lock);
        ::mover.push_back(move);
        ::score.push_back(Fc);
        LeaveCriticalSection(&::lock);
    }
}



//  Fcフェーズソルバー
TEMP void solveFc( int id, Field<PARAM> F, vector<MOVE> *move, int *Fc )
{
    Field<PARAM> Finit = F;

    //  直前の状態と、その状態からの指手
    vector<vector<pair<int,MOVE>>> Hist;
    //  ビームサーチのノード
    vector<NODE<PARAM>> Tf;
    Tf.push_back( NODE<PARAM>( F.getFc(), F.getstate(), -1, MOVE(0,0) ) );

    for ( int n=0; n<SML(FC_STEP,W); n++ )
    {
        checkTerminate(id);

        vector<NODE<PARAM>> Pf = Tf;
        Tf.clear();

        for ( int i=0; i<(int)Pf.size(); i++ )
        {
            F.setstate(Pf[i].state);

            for ( int x=-T; x<W; x++ )
            for ( int r=0; r<4; r++ )
            if ( F.check( MOVE(x,r) ) )
            {
                F.move( MOVE(x,r) );
                Tf.push_back( NODE<PARAM>( F.getFc(), F.getstate(), i, MOVE(x,r) ) );
                F.undo();
            }
        }

        random_shuffle( Tf.begin(), Tf.end() );
        sort( Tf.begin(), Tf.end() );
        reverse( Tf.begin(), Tf.end() );

        if ( Tf.size() > SML(FC_WIDTH,W) )
            Tf.resize( SML(FC_WIDTH,W) );

        Hist.push_back( vector<pair<int,MOVE>>() );
        for ( int i=0; i<(int)Tf.size(); i++ )
            Hist[n].push_back( make_pair(Tf[i].prev,Tf[i].move) );
    }

    move->clear();
    int p=0;
    for ( int n=SML(FC_STEP,W)-1; n>=0; n-- )
        move->push_back(Hist[n][p].second),
        p = Hist[n][p].first;
    reverse(move->begin(),move->end());

    for ( int n=0; n<SML(FC_STEP,W); n++ )
        Finit.move((*move)[n]);
    *Fc = Finit.getFc();
}



//  スコアフェーズのスレッドエントリーポイント
//  moverに手順、scoreにスコアを追加していく
TEMP void __cdecl threadSc( void *arglist )
{
    int id = (int)arglist;
    srand(id);
    
    while ( true )
    {
        Field<PARAM> F(pack);
        for ( int i=0; i<(int)::move.size(); i++ )
            F.move(::move[i]);

        vector<MOVE> move;
        long long score;
        solveSc<PARAM>( id, F, &move, &score );

        //  保存
        EnterCriticalSection(&::lock);
        ::mover.push_back(move);
        ::score.push_back(score);
        LeaveCriticalSection(&::lock);
    }
}



//  スコアフェーズソルバー
TEMP void solveSc( int id, Field<PARAM> F, vector<MOVE> *move, long long *score )
{
    Field<PARAM> Finit = F;

    //  直前の状態と、その状態からの指手
    vector<vector<pair<int,MOVE>>> Hist;
    //  ビームサーチのノード
    vector<NODE<PARAM>> Tf;

    //  最初の1手はランダム
    MOVE m;
    do
        m = MOVE( rand()%(W+T)-T, rand()%4 );
    while ( !F.check(m) );
    F.move(m);

    Hist.push_back(vector<pair<int,MOVE>>());
    Hist[0].push_back(make_pair(0,m));

    Tf.push_back( NODE<PARAM>( F.getidealscore(), F.getstate(), 0, m ) );

    for ( int n=1; n<SML(SC_STEP,W) && Finit.getstep()+n<N; n++ )
    {
        checkTerminate(id);

        vector<NODE<PARAM>> Pf = Tf;
        Tf.clear();

        for ( int i=0; i<(int)Pf.size(); i++ )
        {
            F.setstate(Pf[i].state);

            for ( int x=-T; x<W; x++ )
            for ( int r=0; r<4; r++ )
            if ( F.check(MOVE(x,r)) )
            {
                F.move( MOVE(x,r) );
                Tf.push_back( NODE<PARAM>( F.getidealscore(), F.getstate(), i, MOVE(x,r) ) );
                F.undo();
            }
        }

        random_shuffle( Tf.begin(), Tf.end() );
        sort( Tf.begin(), Tf.end() );
        reverse( Tf.begin(), Tf.end() );

        if ( Tf.size() > SML(SC_WIDTH,W) )
            Tf.resize( SML(SC_WIDTH,W) );

        Hist.push_back(vector<pair<int,MOVE>>());
        for ( int i=0; i<(int)Tf.size(); i++ )
            Hist[n].push_back( make_pair(Tf[i].prev,Tf[i].move) );
    }

    vector<MOVE> mv;
    int p=0;
    for ( int n=(int)Hist.size()-1; n>=0; n-- )
        mv.push_back(Hist[n][p].second),
        p = Hist[n][p].first;
    reverse(mv.begin(),mv.end());

    //  最高のスコアが獲得できたステップまで実行
    F = Finit;

    double maxr = -1e100;
    long long maxscore;
    int maxstep;
    MOVE maxmove;

    for ( int i=0; i<(int)mv.size(); i++ )
    {
        for ( int x=-T; x<W; x++ )
        for ( int r=0; r<4; r++ )
        if ( F.check(MOVE(x,r)) )
        {
            F.move(MOVE(x,r));
            double rr = (double)(F.getscore()-Finit.getscore())/(i+1);

            if ( rr>maxr )
            {
                maxr = rr;
                maxscore = F.getscore()-Finit.getscore();
                maxstep = i+1;
                maxmove = MOVE(x,r);
            }
            
            F.undo();
        }

        F.move(mv[i]);
    }

    mv.resize(maxstep-1);
    mv.push_back(maxmove);

    *move = mv;
    *score = maxscore;
}



//  スレッドの終了が指示されているかチェック
void checkTerminate( int id )
{
    //  終了判定
    EnterCriticalSection(&::lock);
    bool t = false;
    if ( ::term )
    {
        ::finish[id] = true;
        t = true;
    }
    LeaveCriticalSection(&::lock);

    if ( t )
        _endthread();
}
