/*

例: W=5, H=8, T=2

■フィールド

     T↑ 45 46 47 48 49
      ↓ 40 41 42 43 44
     H↑ 35 36 37 38 39
         30 31 32 33 34
         25 26 27 28 29
         20 21 22 23 24
         15 16 17 18 19
         10 11 12 13 14
          5  6  7  8  9
      ↓  0  1  2  3  4
        ← W         →
     ※プログラム中のHは入力のH+T

■パック
     T↑  2  3
      ↓  0  1
        ← T→

■回転
    パックPを回転Rしたときの位置pにあるブロックは
      P[pspin[R][p]]
    で得られる。

    pspini[4][T*T] = {
        { 0, 1, 2, 3 },
        { 1, 3, 0, 2 },
        { 3, 2, 1, 0 },
        { 2, 0, 3, 1 }}

■ライン
　  消去の際の、縦・横・斜めのラインに属するブロックは
    等差数列で表せるので、初項・末項の次の項・公差で管理する。
    落下に使い回すため、最初のW本は縦方向。

    line[][4] = {
        { 0,50, 5}, { 1,51, 5}, …, { 4,54, 5},  //  W本
        { 0, 5, 1}, { 5,10, 1}, …, {45,50, 1},  //  H本
        { 4,10, 6}, { 3,15, 6}, …, {45,51, 6},  //  W+H-1本
        { 0, 4, 4}, { 1, 9, 4}, …, {49,53, 4}}  //  W+H-1本

■ライン逆変換
    各ブロックが属するライン。
    最初の要素は縦方向。

    pos2line[][4] = {
        { 0, 5,19,29}, {1,5,18,30}, … };
*/

#include    "field.h"
#include    <cstdio>
#include    <iostream>

using namespace std;



//  初期化
//  p[n][y][x]: パック、左下が0,0
TEMP Field<PARAM>::Field( vector<vector<vector<int>>> p )
{
    for ( int i=0; i<N; i++ )
        for ( int y=0; y<T; y++ )
        for ( int x=0; x<T; x++ )
            pack[i][y*T+x] = (char)p[i][y][x];

    memset( F, 0, sizeof F );
    Fc = 0;
    step = 0;
    score = 0LL;
    blocknum = 0;
    memset( height, 0, sizeof height );

    for ( int i=0; i<N; i++ )
    for ( int R=0; R<4; R++ )
    {
        int l = T;
        int r = 0;
        for ( int p=0; p<T*T; p++ )
        if ( pack[i][pspin[R][p]]>0 )
            l = min( l, p%T ),
            r = max( r, p%T );
        pleft[i][R] = -l;
        pright[i][R] = W-r-1;
    }
}



//  mが範囲内かどうかチェック
TEMP bool Field<PARAM>::check( MOVE m ) const
{
    return pleft[step][m.R] <= m.X && m.X <= pright[step][m.R];
}



//  指定位置にパックを落とす
TEMP void Field<PARAM>::move( MOVE m )
{
    char p[T*T];
    for ( int i=0; i<T*T; i++ )
        p[i] = pack[step][pspin[m.R][i]];

    move( p, m.X );
}



//  1手戻す
TEMP void Field<PARAM>::undo()
{
    step--;
    if ( step<N )
    {
        memcpy( F, hist_F[step], sizeof F );
        Fc = hist_Fc[step];
        score = hist_score[step];
        blocknum = hist_blocknum[step];
        memcpy( height, hist_height[step], sizeof height );
    }
}



//  履歴以外の状態を保存
TEMP STATE<PARAM> Field<PARAM>::getstate() const
{
    STATE<PARAM> s;
    memcpy( s.F, F, sizeof F );
    s.Fc = Fc;
    s.step = step;
    s.score = score;
    s.blocknum = blocknum;
    memcpy( s.height, height, sizeof height );
    return s;
}



//  履歴以外の状態を設定
TEMP void Field<PARAM>::setstate( const STATE<PARAM> &s )
{
    memcpy( F, s.F, sizeof F );
    Fc = s.Fc;
    step = s.step;
    score = s.score;
    blocknum = s.blocknum;
    memcpy( height, s.height, sizeof height );
}



//  好きな場所に好きなブロックを1個落とせたら得られる
//  最大スコアと最大連鎖数、その位置、ブロックの数字を取得
TEMP void Field<PARAM>::getideal( long long *score, int *bpos, int *bval )
{
    if ( step>=N )
    {
        if ( score!=NULL ) *score = getscore();
        if ( bpos!=NULL ) *bpos = 0;
        if ( bval!=NULL ) *bval = 0;
        return;
    }

    long long s = -INF;
    int c = -1;

    char p[T*T] = {0};

    for ( int x=0; x<W; x++ )
    for ( int v=1; v<S; v++ )
    {
        p[0] = v;

        move( p, x );

        if ( getscore()>s )
        {
            s = getscore();
            if ( bpos!=NULL )  *bpos = x;
            if ( bval!=NULL )  *bval = v;
        }

        undo();
    }

    if ( score!=NULL ) *score = s;

    return;
}



//  盤面を文字列に変換
//  detailが真なら連鎖数なども出力
TEMP string Field<PARAM>::tostring()// const
{
    string R;
    char tmp[32];

    sprintf( tmp, "Turn: %d\n", step+1 );  R += tmp;
    sprintf( tmp, "Score: %g\n", (double)score );  R += tmp;
    
    int bpos, bval;
    long long score;
    getideal( &score, &bpos, &bval );
    sprintf( tmp, "MaxScore: %g\n", (double)score );  R += tmp;
    sprintf( tmp, "BlockNum: %d\n", getblocknum() );  R += tmp;
    sprintf( tmp, "Fc: %d\n", Fc );  R += tmp;

    for ( int i=0; i<bpos; i++ )
        R += "   ";
    sprintf( tmp, " %2d", bval );
    R += tmp;
    R += "\n";

    for ( int y=H-T-1; y>=0; y-- )
    {
        for ( int x=0; x<W; x++ )
        {
            if ( F[y*W+x]==0 )
                R += "  .";
            else if ( F[y*W+x]>S )
                R += " ##";
            else
                sprintf( tmp, " %2d", F[y*W+x] ),
                R += tmp;
        }
        R += "\n";
    }
    return R;
}



//  指定位置にパックを落とす
TEMP void Field<PARAM>::move( const char p[T*T], int xpos )
{
    if ( step>=N )
    {
        step++;
        return;
    }

    //  履歴保存
    memcpy( hist_F[step], F, sizeof F );
    hist_Fc[step] = Fc;
    hist_score[step] = score;
    hist_blocknum[step] = blocknum;
    memcpy( hist_height[step], height, sizeof height );

    if ( score<=-INF )
    {
        step++;
        return;
    }

    //  ライン
    //  updateb[l]!=LINFならば、
    //  updateb[l]<=,<updatee[l]のブロックが変化している
    //  updatef[l]!=LINFならば
    //  updatef[l]以上に空白がある
    const int LINF = 9999;
    int updateb[linenum];
    int updatee[linenum];
    int updatef[W];
    for ( int l=0; l<linenum; l++ )
        updateb[l] = +LINF,
        updatee[l] = -LINF;
    for ( int l=0; l<W; l++ )
        updatef[l] = LINF;

    //  置く
    for ( int y=0; y<T; y++ )
    for ( int x=0; x<T; x++ )
    if ( p[y*T+x] > 0 )
    {
        int t = height[x+xpos]*W+(x+xpos);
        F[t] = p[y*T+x];
        blocknum++;
        height[x+xpos]++;
        for ( int i=0; i<4; i++ )
        {
            int lu = pos2line[t][i];
            updateb[lu] = min( updateb[lu], t );
            updatee[lu] = max( updatee[lu], t+line[lu][2] );
        }
    }

    long long raw_score = 0LL;

    for ( int C=1; ; C++ )
    {
        //  削除
        int E = 0;
        bool Fe[W*H] = {false};     //  削除するブロック

        //  値がSのブロックを削除
        if ( C==1 )
        {
            for ( int l=0; l<W; l++ )
            if ( updateb[l]!=LINF )
            {
                for ( int p=updateb[l]; p!=updatee[l]; p+=line[l][2] )
                if ( F[p]==S )
                {
                    F[p] = 0;
                    E++;
                    updatef[l] = min( updatef[l], p );
                    blocknum--;

                    //  お邪魔ブロック
                    for ( int d=0; d<8; d++ )
                    {
                        int pp = neighbor8[p][d];
                        if ( F[pp]==S+1 )
                        {
                            F[pp] = 0;
                            E++;
                            int px = pos2line[pp][0];
                            updatef[px] = min( updatef[px], pp );
                            blocknum--;
                        }
                    }
                }
            }
        }

        //  その他のブロックを削除
        for ( int l=0; l<linenum; l++ )
        if ( updateb[l]!=LINF )
        {
            int b = line[l][0];
            int e = line[l][1];
            int d = line[l][2];

            //  開始位置はupdateb[l]から戻って合計がS以下の所
            int p1 = updateb[l];
            int s = 0;
            while ( p1!=b && s<S )
                s += F[p1],
                p1 -= d;
            int p2 = p1;

            while ( p2!=e && p1!=updatee[l] )
            {
                //  ブロックがあるところまで進める
                while ( p2!=e && F[p2]==0 )
                    p2 += d;
                if ( p2==e )
                    break;
                p1 = p2;

                //  しゃくとり法
                int s = 0;
                while ( p2!=e && p1!=updatee[l] && F[p2]>0 || s>S )
                {
                    if ( s<S )
                    {
                        s += F[p2];
                        p2 += d;
                    }
                    else
                    {
                        s -= F[p1];
                        p1 += d;
                    }

                    if ( s==S )
                    {
                        for ( int p=p1; p!=p2; p+=d )
                        {
                            Fe[p] = true;
                            E++;
                            int lu = pos2line[p][0];
                            updatef[lu] = min( updatef[lu], p );

                            //  お邪魔ブロック
                            for ( int d=0; d<8; d++ )
                            {
                                int pp = neighbor8[p][d];
                                if ( F[pp]==S+1 && !Fe[pp] )
                                {
                                    Fe[pp] = true;
                                    E++;
                                    int px = pos2line[pp][0];
                                    updatef[px] = min( updatef[px], pp );
                                }
                            }
                        }
                    }
                }
            }

            updateb[l] = +LINF;
            updatee[l] = -LINF;
        }

        //  消すブロックが無ければ、連鎖数を記録して終了
        if ( E==0 )
            break;

        //  実際にブロックを消す
        for ( int x=0; x<W; x++ )
        if ( updatef[x]!=LINF )
        {
            int e = height[x]*W+x;
            for ( int p=updatef[x]; p!=e; p+=line[x][2] )
            if ( Fe[p] )
            {
                F[p] = 0;
                blocknum--;
            }
        }

        //  スコア更新
        raw_score += (1LL<<min(E/3,P)) * max(1,E/3-P+1) * C;

        //  落下
        for ( int l=0; l<W; l++ )
        if ( updatef[l]!=LINF )
        {
            int p1 = updatef[l];
            int d = line[l][2];
            int e = height[l]*W+l;
            int p2 = p1;

            while ( true )
            {
                while ( p2!=e && F[p2]==0 )
                    p2 += d;
                if ( p2==e )
                    break;

                F[p1] = F[p2];
                F[p2] = 0;
                for ( int i=0; i<4; i++ )
                {
                    int lu = pos2line[p1][i];
                    updateb[lu] = min( updateb[lu], p1 );
                    updatee[lu] = max( updatee[lu], p1+line[lu][2] );
                }
                p1 += d;
            }

            height[l] = p1/W;

            updatef[l] = LINF;
        }
    }

    //  スコア更新
    score += raw_score*(Fc+1);
    if ( raw_score>=Th )
        Fc++;

    //  ゲームオーバーチェック
    for ( int x=0; x<W; x++ )
        if ( height[x]>H-T )
            score = -INF;

    step++;
}



//  明示的インスタンス化

#pragma warning(disable:4661)   // 明示的なテンプレートのインスタンス生成要求に対して適切な定義がありません。
template class Field<  5,  8+2, 2, 10,   10, 25,   100>;        //  Test
template class Field< 10, 16+4, 4, 10, 1000, 25,   100>;        //  Small
template class Field< 15, 23+4, 4, 20, 1000, 30,  1000>;        //  Medium
template class Field< 20, 36+5, 5, 30, 1000, 35, 10000>;        //  Large
