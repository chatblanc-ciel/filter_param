/*
 * main.cpp
 *
 *  Created on: 2021/07/22
 *      Author: matsu
 */

#include "cascade_iir.hpp"

#include <assert.h>
#include <chrono>
#include <cstdio>
#include <string>
#include <iostream>
#include <cstdlib>
#include <exception>
#include <DspWorkUtils/stack_backtrace>

#ifdef _MSC_VER
    #include <windows.h>
#endif

void my_terminate_handler() {
    try {
        std::cerr << boost::stacktrace::stacktrace();
    } catch (...) {}
    std::abort();
}

using namespace std;
using namespace filter::iir;

void test_BandParam_new();
void test_Band_generator();
void test_analyze_edges();
void test_FilterParam_read_csv();
void run_FilterParam_new_single_band();
void run_FilterParam_new_multi_band();
void test_FilterParam_csw();
void test_FilterParam_desire_res();
void test_FilterParam_freq_res_speed();
void test_FilterParam_freq_res_se();
void test_FilterParam_freq_res_so();
void test_FilterParam_freq_res_no();
void test_FilterParam_freq_res_mo();
void test_Filter_param_group_delay_se();
void test_Filter_param_group_delay_so();
void test_Filter_param_group_delay_no();
void test_Filter_param_group_delay_mo();
void test_FilterParam_judge_stability_even();
void test_FilterParam_judge_stability_odd();
void test_FilterParam_evaluate_objective_function();
void test_FilterParam_init_coef();
void test_FilterParam_init_stable_coef();
void test_FilterParam_gprint_amp();
void test_FilterParam_gprint_mag();
void test_FilterParam_pole_even();
void test_FilterParam_pole_odd();
void test_FilterParam_zero_even();
void test_FilterParam_zero_odd();

int main( int argc, char** argv )
{
    std::set_terminate(&my_terminate_handler);
    vector< string > args( argv, argv + argc );

    if ( args.at( 1 ) == string( "BandParam_new" ) )
    {
        test_BandParam_new();
    }
    else if ( args.at( 1 ) == string( "Band_generator" ) )
    {
        test_Band_generator();
    }
    else if ( args.at( 1 ) == string( "analyze_edges" ) )
    {
        test_analyze_edges();
    }
    else if ( args.at( 1 ) == string( "FilterParam_new_single_band" ) )
    {
        run_FilterParam_new_single_band();
    }
    else if ( args.at( 1 ) == string( "FilterParam_new_multi_band" ) )
    {
        run_FilterParam_new_multi_band();
    }
    else if ( args.at( 1 ) == string( "FilterParam_read_csv" ) )
    {
        test_FilterParam_read_csv();
    }
    else if ( args.at( 1 ) == string( "FilterParam_csw" ) )
    {
        test_FilterParam_csw();
    }
    else if ( args.at( 1 ) == string( "FilterParam_desire_res" ) )
    {
        test_FilterParam_desire_res();
    }
    else if ( args.at( 1 ) == string( "FilterParam_freq_res_se" ) )
    {
        test_FilterParam_freq_res_se();
    }
    else if ( args.at( 1 ) == string( "FilterParam_freq_res_so" ) )
    {
        test_FilterParam_freq_res_so();
    }
    else if ( args.at( 1 ) == string( "FilterParam_freq_res_no" ) )
    {
        test_FilterParam_freq_res_no();
    }
    else if ( args.at( 1 ) == string( "FilterParam_freq_res_mo" ) )
    {
        test_FilterParam_freq_res_mo();
    }
    else if ( args.at( 1 ) == string( "Filter_param_group_delay_se" ) )
    {
        test_Filter_param_group_delay_se();
    }
    else if ( args.at( 1 ) == string( "Filter_param_group_delay_so" ) )
    {
        test_Filter_param_group_delay_so();
    }
    else if ( args.at( 1 ) == string( "Filter_param_group_delay_no" ) )
    {
        test_Filter_param_group_delay_no();
    }
    else if ( args.at( 1 ) == string( "Filter_param_group_delay_mo" ) )
    {
        test_Filter_param_group_delay_mo();
    }
    else if ( args.at( 1 ) == string( "FilterParam_judge_stability_even" ) )
    {
        test_FilterParam_judge_stability_even();
    }
    else if ( args.at( 1 ) == string( "FilterParam_judge_stability_odd" ) )
    {
        test_FilterParam_judge_stability_odd();
    }
    else if (
        args.at( 1 ) == string( "FilterParam_evaluate_objective_function" ) )
    {
        test_FilterParam_evaluate_objective_function();
    }
    else if ( args.at( 1 ) == string( "FilterParam_init_coef" ) )
    {
        test_FilterParam_init_coef();
    }
    else if ( args.at( 1 ) == string( "FilterParam_init_stable_coef" ) )
    {
        test_FilterParam_init_stable_coef();
    }
    else if ( args.at( 1 ) == string( "FilterParam_gprint_amp" ) )
    {
        test_FilterParam_gprint_amp();
    }
    else if ( args.at( 1 ) == string( "FilterParam_gprint_mag" ) )
    {
        test_FilterParam_gprint_mag();
    }
    else
    {
        fprintf( stderr, "Matching test is not exist.\n" );
        exit( -1 );
    }

    return 0;
}

/* 周波数帯域の構造体
 *   生成と表示のテスト
 *
 */
void test_BandParam_new()
{
    auto bp = BandParam( BandType::Pass, 0.0, 0.2175 );
    printf( "%s\n", bp.sprint().c_str() );
}

/* フィルタ構造体
 *   フィルタタイプから周波数帯域を生成するテスト
 *   大抵、複数の周波数帯域からフィルタが成るため、
 *   vectorで周波数帯域を返却する
 */
void test_Band_generator()
{
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.2, 0.3 );
    for ( auto bp : bands )
    {
        printf( "%s\n", bp.sprint().c_str() );
    }
}

/* フィルタ構造体
 *   文字列(string)から、フィルタタイプと
 *   周波数帯域端を分離するテスト
 */
void test_analyze_edges()
{
    string type( "LPF(0.2 : 0.3)" );
    auto ftype = FilterParam::analyze_type( type );
    auto edges = FilterParam::analyze_edges( type );

    if ( ftype == FilterType::LPF )
    {
        printf( "LPF\n" );
    }
    for ( auto v : edges )
    {
        printf( "%f ", v );
    }
    printf( "\n" );
}

/* フィルタ構造体
 *   CSVファイルから所望特性を読み取るテスト
 *   書式は以下の通り
 *
 *     No,Numerator,Denominator,State,GroupDelay,NsplitApprox,NspritTransition
 *
 *   vectorのインデックス = Noなので
 *   "No"は読みこまない
 */
void test_FilterParam_read_csv()
{
#ifdef _MSC_VER
    char Path[MAX_PATH + 1];

    printf( "check full path\n" );
    if ( 0 != GetModuleFileName( NULL, Path, MAX_PATH ) )
    {    // 実行ファイルの完全パスを取得

        char drive[MAX_PATH + 1], dir[MAX_PATH + 1], fname[MAX_PATH + 1], ext[MAX_PATH + 1];

        //パス名を構成要素に分解します
        _splitpath_s( Path, drive, sizeof( drive ), dir, sizeof( dir ), fname, sizeof( fname ), ext, sizeof( ext ) );

    #ifdef DEBUG
        printf( "完全パス : %s\n", Path );
        printf( "ドライブ : %s\n", drive );
        printf( "ディレクトリ パス : %s\n", dir );
        printf( "ベース ファイル名 (拡張子なし) : %s\n", fname );
        printf( "ファイル名の拡張子 : %s\n", ext );
    #endif
        string filename = format( "%s\\%s\\desire_filter.csv", drive, dir );
        auto params = FilterParam::read_csv( filename );
        for ( auto param : params )
        {
            printf(
                "order(zero/pole) : %d/%d\n", param.zero_order(),
                param.pole_order() );
            printf( "optimization order : %d\n", param.opt_order() );
            printf(
                "nsplit(approx-transition) : %d-%d\n", param.partition_approx(),
                param.partition_transition() );
            printf( "group delay : %f\n\n", param.gd() );

            for ( auto band : param.fbands() )
            {
                printf( "%s\n", band.sprint().c_str() );
            }
            printf( "---------------------------\n" );
        }
    }
    else
    {
        exit( EXIT_FAILURE );
    }
#else
    string filename( "./desire_filter.csv" );
    auto params = FilterParam::read_csv( filename );
    for ( auto param : params )
    {
        printf(
            "order(zero/pole) : %d/%d\n", param.zero_order(),
            param.pole_order() );
        printf( "optimization order : %d\n", param.opt_order() );
        printf(
            "nsplit(approx-transition) : %d-%d\n", param.partition_approx(),
            param.partition_transition() );
        printf( "group delay : %f\n\n", param.gd() );

        for ( auto band : param.fbands() )
        {
            printf( "%s\n", band.sprint().c_str() );
        }
        printf( "---------------------------\n" );
    }
#endif
}

void run_FilterParam_new_single_band()
{
    auto band = BandParam( BandType::Pass, 0.0, 0.2 );
    FilterParam fparam( 8, 2, band, 200, 50, 5.0 );
}

void run_FilterParam_new_multi_band()
{
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.2, 0.3 );
    FilterParam fparam( 8, 2, bands, 200, 50, 5.0 );
}

/* フィルタ構造体
 *   複素正弦波の１次・２次の確認用
 */
void test_FilterParam_csw()
{
    auto band = BandParam( BandType::Pass, 0.0, 0.2 );
    auto csw = FilterParam::gen_csw( band, 100 );
    auto csw2 = FilterParam::gen_csw2( band, 100 );

    for ( auto z : csw )
    {
        printf( "%6f %6f\n", z.real(), z.imag() );
    }
    printf( "\n\n\n\n" );
    for ( auto z : csw2 )
    {
        printf( "%6f %6f\n", z.real(), z.imag() );
    }
}

/* # フィルタ構造体
 *   所望特性の周波数特性についてのテスト関数
 *   通過域でe^jωτ(τ= group delay)，
 *   阻止域で０，遷移域で要素なしの出力
 */
void test_FilterParam_desire_res()
{
    double gd = 5.0;

    auto pass_band = BandParam( BandType::Pass, 0.0, 0.2 );
    auto desire_pass = FilterParam::gen_desire_res( pass_band, 100, gd );
    printf( "-----------pass band-----------------\n" );
    for ( auto z : desire_pass )
    {
        printf( "%6f %6f\n", z.real(), z.imag() );
    }
    printf( "----------------------------\n\n" );

    auto stop_band = BandParam( BandType::Stop, 0.0, 0.2 );
    auto desire_stop = FilterParam::gen_desire_res( stop_band, 100, gd );
    printf( "-----------stop band-----------------\n" );
    for ( auto z : desire_stop )
    {
        printf( "%6f %6f\n", z.real(), z.imag() );
    }
    printf( "----------------------------\n\n" );

    auto trans_band = BandParam( BandType::Transition, 0.0, 0.2 );
    auto desire_trans = FilterParam::gen_desire_res( trans_band, 100, gd );
    printf( "-----------transition band-----------------\n" );
    for ( auto z : desire_trans )
    {
        printf( "%6f %6f\n", z.real(), z.imag() );
    }
    printf( "----------------------------\n\n" );
}

/* # フィルタ構造体
 *   周波数特性計算関数の実行速度を計算する
 *
 *   trialについての平均で判断する
 *   また各１回の実行時間は微小のため，
 *   repeat分繰り返して割ることで
 *   精度よく測定することを試みる
 */
void test_FilterParam_freq_res_speed()
{
    printf( "thread will ce locked about 2 minutes.\n" );

    // 時間計測関連
    std::size_t trial = 100;
    int exp_ = 5;
    std::size_t repeat = static_cast< std::size_t >( std::pow( 10, exp_ ) );
    double ave = 0.0;
    double ave_all = 0.0;

    // 計測雑利用
    vector< double > coef { 0.018656458,

                            1.969338828, 1.120102082, 0.388717952,
                            0.996398946, 1.048137529, 1.037079725,
                            -4.535575709, 6.381429398,

                            -0.139429968, 0.763426685 };
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.2, 0.3 );
    FilterParam fparam( 8, 2, bands, 200, 50, 5.0 );
    vector< vector< complex< double > > > freq_res;

    for ( std::size_t i = 0; i < trial; ++i )
    {
        auto start1 = chrono::system_clock::now();    // 計測スタート時刻を保存

        for ( std::size_t j = 0; j < repeat; ++j )
        {
            freq_res = fparam.freq_res( coef );
        }

        auto end1 = chrono::system_clock::now();    // 計測終了時刻を保存
        int64_t msec1 =
            chrono::duration_cast< chrono::milliseconds >( end1 - start1 )
                .count();
        double once_time1 =
            static_cast< double >( msec1 ) / static_cast< double >( repeat );
        ave += once_time1;
        ave_all += static_cast< double >( msec1 );
    }

    printf( "\n------------------------------------\n\n\n\n" );
    printf(
        "using functional : Average %15.15f[ns]\n",
        1000 * 1000 * ave / static_cast< double >( trial ) );
    printf(
        "using functional : All(10^%d) %15.15f[ms]\n", exp_,
        ave_all / static_cast< double >( trial ) );
    printf(
        "Size : %llu\n",
        static_cast< long long unsigned int >( sizeof( fparam ) ) );
}

/* フィルタ構造体
 *   偶数次/偶数次の場合の周波数特性確認用
 *
 */
void test_FilterParam_freq_res_se()
{
    vector< double > coef { 0.018656458,

                            1.969338828, 1.120102082, 0.388717952,
                            0.996398946, 1.048137529, 1.037079725,
                            -4.535575709, 6.381429398,

                            -0.139429968, 0.763426685 };
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.2, 0.3 );
    FilterParam fparam( 8, 2, bands, 200, 50, 5.0 );

    auto freq_res = fparam.freq_res( coef );

    for ( auto band_res : freq_res )
    {
        for ( auto res : band_res )
        {
            printf( "%f\n", abs( res ) );
        }
    }
}

void test_FilterParam_freq_res_so()
{
    vector< double > coef { -0.040659737, -2.372311969,

                            -2.144646171, 4.343497453, 1.359348897,
                            0.984834163,

                            -0.710147059, -0.696696684, 0.514853197,
                            0.503697311, 0.70680348 };

    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.3, 0.345 );
    FilterParam fparam( 5, 5, bands, 200, 50, 5.0 );

    auto freq_res = fparam.freq_res( coef );

    for ( auto band_res : freq_res )
    {
        for ( auto res : band_res )
        {
            printf( "%f\n", abs( res ) );
        }
    }
}

void test_FilterParam_freq_res_no()
{
    vector< double > coef { 0.025247504683641238,

                            0.8885952985540255, -4.097963802039866,
                            5.496940685423355, 0.3983519261092186,
                            0.9723236917140877, 1.1168784833810899,
                            0.8492039597182939,

                            -0.686114259307724, 0.22008381076439384,
                            -0.22066728558327908, 0.7668032045079851 };
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.2, 0.275 );
    FilterParam fparam( 7, 4, bands, 200, 50, 5.0 );

    vector< vector< complex< double > > > freq = fparam.freq_res( coef );

    for ( auto band : freq )
    {
        for ( auto amp : band )
        {
            printf( "%f\n", abs( amp ) );
        }
    }
}

void test_FilterParam_freq_res_mo()
{
    vector< double > coef { -0.040404875,

                            0.957674103, 0.765466003, -1.585891794,
                            -1.903482473, -0.441904071, 0.79143639,
                            -1.149627531, 0.965348065,

                            -0.434908839, -1.332562129, 0.838349784 };
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.1, 0.145 );
    FilterParam fparam( 8, 3, bands, 200, 50, 5.0 );

    auto freq_res = fparam.freq_res( coef );

    for ( auto band_res : freq_res )
    {
        for ( auto res : band_res )
        {
            printf( "%f\n", abs( res ) );
        }
    }
}

void test_Filter_param_group_delay_se()
{
    vector< double > coef { 0.018656458,

                            1.969338828, 1.120102082, 0.388717952,
                            0.996398946, 1.048137529, 1.037079725,
                            -4.535575709, 6.381429398,

                            -0.139429968, 0.763426685 };
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.2, 0.3 );
    FilterParam fparam( 8, 2, bands, 200, 50, 5.0 );

    auto group_delay_res = fparam.group_delay_res( coef );

    for ( auto band_res : group_delay_res )
    {
        for ( auto res : band_res )
        {
            printf( "%f\n", res );
        }
    }
}

void test_Filter_param_group_delay_so()
{
    vector< double > coef { -0.040659737,

                            -2.372311969, -2.144646171, 4.343497453,
                            1.359348897, 0.984834163,

                            -0.710147059, -0.696696684, 0.514853197,
                            0.503697311, 0.70680348 };

    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.3, 0.345 );
    FilterParam fparam( 5, 5, bands, 200, 50, 5.0 );

    auto group_delay_res = fparam.group_delay_res( coef );

    for ( auto band_res : group_delay_res )
    {
        for ( auto res : band_res )
        {
            printf( "%f\n", res );
        }
    }
}

void test_Filter_param_group_delay_no()
{
    vector< double > coef { 0.025247504683641238,

                            0.8885952985540255, -4.097963802039866,
                            5.496940685423355, 0.3983519261092186,
                            0.9723236917140877, 1.1168784833810899,
                            0.8492039597182939,

                            -0.686114259307724, 0.22008381076439384,
                            -0.22066728558327908, 0.7668032045079851 };
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.2, 0.275 );
    FilterParam fparam( 7, 4, bands, 200, 50, 5.0 );

    auto group_delay_res = fparam.group_delay_res( coef );

    for ( auto band : group_delay_res )
    {
        for ( auto res : band )
        {
            printf( "%f\n", res );
        }
    }
}

void test_Filter_param_group_delay_mo()
{
    vector< double > coef { -0.040404875,

                            0.957674103, 0.765466003, -1.585891794,
                            -1.903482473, -0.441904071, 0.79143639,
                            -1.149627531, 0.965348065,

                            -0.434908839, -1.332562129, 0.838349784 };
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.1, 0.145 );
    FilterParam fparam( 8, 3, bands, 200, 50, 5.0 );

    auto group_delay_res = fparam.group_delay_res( coef );

    for ( auto band_res : group_delay_res )
    {
        for ( auto res : band_res )
        {
            printf( "%f\n", res );
        }
    }
}

void test_FilterParam_judge_stability_even()
{
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.2, 0.275 );
    FilterParam fparam( 7, 4, bands, 200, 50, 5.0 );

    vector< double > coef_1    //安定テスト
        { 0.025247504683641238,

          0.8885952985540255, -4.097963802039866, 5.496940685423355,
          0.3983519261092186, 0.9723236917140877, 1.1168784833810899,
          0.8492039597182939,

          0.686114259307724, 0.22008381076439384, -0.22066728558327908,
          0.7668032045079851 };
    double penalty = fparam.judge_stability( coef_1 );
    printf( "stable %f\n", penalty );

    vector< double > coef_2    // b_2>1の時
        { 0.025247504683641238,

          0.8885952985540255, -4.097963802039866, 5.496940685423355,
          0.3983519261092186, 0.9723236917140877, 1.1168784833810899,
          0.8492039597182939,

          0.686114259307724, 1.22008381076439384, -0.22066728558327908,
          0.7668032045079851 };
    penalty = fparam.judge_stability( coef_2 );
    printf( "unstable(b_2>1) %f\n", penalty );

    vector< double > coef_3    // b_1-1>b_2の時
        { 0.025247504683641238,

          0.8885952985540255, -4.097963802039866, 5.496940685423355,
          0.3983519261092186, 0.9723236917140877, 1.1168784833810899,
          0.8492039597182939,

          1.686114259307724, 0.22008381076439384, -0.22066728558327908,
          0.7668032045079851 };
    penalty = fparam.judge_stability( coef_3 );
    printf( "unstable(b_1-1>b_2) %f\n", penalty );

    vector< double > coef_4    //両方不安定の場合
        { 0.025247504683641238,

          0.8885952985540255, -4.097963802039866, 5.496940685423355,
          0.3983519261092186, 0.9723236917140877, 1.1168784833810899,
          0.8492039597182939,

          2.686114259307724, 1.22008381076439384, -0.22066728558327908,
          0.7668032045079851 };

    penalty = fparam.judge_stability( coef_4 );
    printf( "unstable(b_2>1,b_1-1>b_2) %f\n", penalty );
}

void test_FilterParam_judge_stability_odd()
{
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.1, 0.145 );
    FilterParam fparam( 8, 3, bands, 200, 50, 5.0 );
    double penalty = 0.0;

    vector< double > coef_test1 { -0.040404875,

                                  0.957674103, 0.765466003, -1.585891794,
                                  -1.903482473, -0.441904071, 0.79143639,
                                  -1.149627531, 0.965348065,

                                  -0.434908839, -1.332562129, 0.838349784 };

    penalty = fparam.judge_stability( coef_test1 );
    printf( "stability %f\n", penalty );

    vector< double > coef_test2 { -0.040404875,

                                  0.957674103, 0.765466003, -1.585891794,
                                  -1.903482473, -0.441904071, 0.79143639,
                                  -1.149627531, 0.965348065,

                                  -1.434908839, -1.332562129, 0.838349784 };

    penalty = fparam.judge_stability( coef_test2 );
    printf( "instability %f\n", penalty );

    vector< double > coef_test3 { -0.040404875,

                                  0.957674103, 0.765466003, -1.585891794,
                                  -1.903482473, -0.441904071, 0.79143639,
                                  -1.149627531, 0.965348065,

                                  -0.434908839, -1.332562129, 1.838349784 };

    penalty = fparam.judge_stability( coef_test3 );
    printf( "instability %f\n", penalty );

    vector< double > coef_test4 { -0.040404875,

                                  0.957674103, 0.765466003, -1.585891794,
                                  -1.903482473, -0.441904071, 0.79143639,
                                  -1.149627531, 0.965348065,

                                  -0.434908839, -1.332562129, 0.238349784 };

    penalty = fparam.judge_stability( coef_test4 );
    printf( "stability %f\n", penalty );

    vector< double > coef_test5 { -0.040404875,

                                  0.957674103, 0.765466003, -1.585891794,
                                  -1.903482473, -0.441904071, 0.79143639,
                                  -1.149627531, 0.965348065,

                                  -0.434908839, -2.332562129, 1.238349784 };

    penalty = fparam.judge_stability( coef_test5 );
    printf( "instability %f\n", penalty );
}

void test_FilterParam_evaluate_objective_function()
{
    vector< double > coef { 0.025247504683641238,

                            0.8885952985540255, -4.097963802039866,
                            5.496940685423355, 0.3983519261092186,
                            0.9723236917140877, 1.1168784833810899,
                            0.8492039597182939,

                            -0.686114259307724, 0.22008381076439384,
                            -0.22066728558327908, 0.7668032045079851 };

    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.2, 0.275 );
    FilterParam fparam( 7, 4, bands, 200, 50, 5.0 );

    auto objective_function_value = fparam.evaluate( coef );

    printf( "objective_function_value %f\n", objective_function_value );
}

void test_FilterParam_init_coef()
{
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.2, 0.3 );
    FilterParam fparam( 8, 9, bands, 200, 50, 5.0 );
    double a0 = 0.5;
    double a = 3.0;
    double b = 3.0;

    for ( unsigned int i = 0; i < 20; ++i )
    {
        auto coef = fparam.init_coef( a0, a, b );

        for ( auto c : coef )
        {
            printf( "% 3.3f ", c );
        }
        printf( "\n" );
    }
}

void test_FilterParam_init_stable_coef()
{
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.2, 0.3 );
    FilterParam fparam( 2, 9, bands, 200, 50, 5.0 );
    double a0 = 0.5;
    double a = 3.0;

    for ( unsigned int i = 0; i < 20; ++i )
    {
        auto coef = fparam.init_stable_coef( a0, a );

        printf( "[ %3.3f]", fparam.judge_stability( coef ) );
        for ( auto c : coef )
        {
            printf( "% 3.3f ", c );
        }
        printf( "\n" );
    }
}

/* フィルタ構造体
 * 振幅特性図の描画
 * leftとrightで描画範囲の指定[0:0.5]
 *
 */
void test_FilterParam_gprint_amp()
{
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.2, 0.275 );
    FilterParam fparam( 7, 4, bands, 200, 50, 5.0 );

    vector< double > coef_test { 0.025247504683641238,

                                 0.8885952985540255, -4.097963802039866,
                                 5.496940685423355, 0.3983519261092186,
                                 0.9723236917140877, 1.1168784833810899,
                                 0.8492039597182939,

                                 -0.686114259307724, 0.22008381076439384,
                                 -0.22066728558327908, 0.7668032045079851 };

    // BandParamが共に[0.0 : 0.5]の範囲内＆``left < right``のため成功
    fparam.gprint_amp( coef_test, string( "Amp1.png" ), 0.0, 0.3 );

    fparam.gprint_amp( coef_test, string( "Amp2.png" ), 0.0, 0.5 );

    fparam.gprint_amp( coef_test, string( "Amp3.png" ), 0.2, 0.5 );

    fparam.gprint_amp( coef_test, string( "Amp4.png" ), 0.2, 0.3 );

    // 以下失敗例
    // fparam.gprint_amp(coef_test, string("Amp5.png"), -0.2, 0.3);
    // //BandParamよりleft<0.0のため失敗

    // fparam.gprint_amp(coef_test, string("Amp6.png"), 0.3, 0.2);
    // //BandParamよりleft>rightのため失敗

    // fparam.gprint_amp(coef_test, string("Amp7.png"), 0.2, 0.6);
    // //BandParamよりright>0.5のため失敗
}

void test_FilterParam_gprint_mag()
{
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.2, 0.275 );
    FilterParam fparam( 7, 4, bands, 200, 50, 5.0 );

    vector< double > coef_test { 0.025247504683641238,

                                 0.8885952985540255, -4.097963802039866,
                                 5.496940685423355, 0.3983519261092186,
                                 0.9723236917140877, 1.1168784833810899,
                                 0.8492039597182939,

                                 -0.686114259307724, 0.22008381076439384,
                                 -0.22066728558327908, 0.7668032045079851 };

    // BandParamが共に[0.0 : 0.5]の範囲内＆``left < right``のため成功
    fparam.gprint_mag( coef_test, string( "Mag1.png" ), 0.0, 0.3 );

    fparam.gprint_mag( coef_test, string( "Mag2.png" ), 0.0, 0.5 );

    fparam.gprint_mag( coef_test, string( "Mag3.png" ), 0.2, 0.5 );

    fparam.gprint_mag( coef_test, string( "Mag4.png" ), 0.2, 0.3 );

    // 以下失敗例
    // fparam.gprint_mag(coef_test, string("Mag5.png"), -0.2, 0.3);
    // //BandParamよりleft<0.0のため失敗

    // fparam.gprint_mag(coef_test, string("Mag6.png"), 0.3, 0.2);
    // //BandParamよりleft>rightのため失敗

    // fparam.gprint_mag(coef_test, string("Mag7.png"), 0.2, 0.6);
    // //BandParamよりright>0.5のため失敗
}

/* フィルタ構造体
 * 分母の係数列から極、分子の係数列から零点の計算を
 * 係数列数が奇数の場合と偶数の場合で行う
 *
 */
void test_FilterParam_pole_even()
{
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.2, 0.3 );
    FilterParam fparam( 0, 6, bands, 200, 50, 5.0 );

    vector< double > coef {
        0.018656458,

        -4.097963802039866,         // 複素解の組み合わせ
        5.496940685423355,          // 複素解の組み合わせ
        -1.585891794,               // 実数解の組み合わせ
        -1.903482473,               // 実数解の組み合わせ
        1.1168784833810899,         // 重解の組み合わせ
        0.3118543866599108769856    // 重解の組み合わせ
    };

    auto pole_res = fparam.pole_res( coef );

    for ( auto pole : pole_res )
    {
        printf( "%.15f %.15f\n", pole.real(), pole.imag() );
    }

    const double acc = 1.0e-10;    // 1.0×10^-10≒0とし、これを基準に場合分けを行う

    std::vector< complex< double > > test_pole;
    test_pole.reserve( 5 );

    for ( unsigned int m = 1; m < 7; m += 2 )
    {
        double disc = coef.at( m ) * coef.at( m ) - 4.0 * coef.at( m + 1 );

        if ( disc < -acc )    // 複素解
        {
            double disc_sqrt = sqrt( -disc );
            complex< double > plus_test_pole( -coef.at( m ) / 2.0, disc_sqrt / 2.0 );
            complex< double > minus_test_pole( -coef.at( m ) / 2.0, -disc_sqrt / 2.0 );
            test_pole.emplace_back( plus_test_pole );
            test_pole.emplace_back( minus_test_pole );
            printf( "%.15f %.15f\n", plus_test_pole.real(), plus_test_pole.imag() );
            printf( "%.15f %.15f\n", minus_test_pole.real(), minus_test_pole.imag() );
        }
        else if ( disc > acc )    // 実数解
        {
            double disc_sqrt = sqrt( disc );
            complex< double > plus_test_pole( ( -coef.at( m ) + disc_sqrt ) / 2.0, 0.0 );
            complex< double > minus_test_pole( ( -coef.at( m ) - disc_sqrt ) / 2.0, 0.0 );
            test_pole.emplace_back( plus_test_pole );
            test_pole.emplace_back( minus_test_pole );
            printf( "%.15f %.15f\n", plus_test_pole.real(), plus_test_pole.imag() );
            printf( "%.15f %.15f\n", minus_test_pole.real(), minus_test_pole.imag() );
        }
        else if ( disc >= -acc && disc <= acc )    // 重解
        {
            complex< double > multiple_test_pole( -coef.at( m ) / 2.0, 0.0 );
            test_pole.emplace_back( multiple_test_pole );
            printf( "%.15f %.15f\n", multiple_test_pole.real(), multiple_test_pole.imag() );
        }
        else
        {
            exit( -1 );    // 場合分けで漏れた場合エラーが発生
        }
    }

    const unsigned int acc2 = 1000;    // 10^-n(小数点以下n桁)まで精度を検査
    for ( unsigned int n = 0; n < 5; n++ )
    {
        assert( static_cast< size_t >( std::round( pole_res.at( n ).real() * acc2 ) ) == static_cast< size_t >( std::round( test_pole.at( n ).real() * acc2 ) ) );
        assert( static_cast< size_t >( std::round( pole_res.at( n ).imag() * acc2 ) ) == static_cast< size_t >( std::round( test_pole.at( n ).imag() * acc2 ) ) );
    }
}

void test_FilterParam_pole_odd()
{
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.3, 0.345 );
    FilterParam fparam( 0, 7, bands, 200, 50, 5.0 );

    vector< double > coef {
        -0.040659737,

        -0.710147059,
        -4.097963802039866,         // 複素解の組み合わせ
        5.496940685423355,          // 複素解の組み合わせ
        -1.585891794,               // 実数解の組み合わせ
        -1.903482473,               // 実数解の組み合わせ
        1.1168784833810899,         // 重解の組み合わせ
        0.3118543866599108769856    // 重解の組み合わせ
    };

    auto pole_res = fparam.pole_res( coef );

    for ( auto pole : pole_res )
    {
        printf( "%.15f %.15f\n", pole.real(), pole.imag() );
    }

    const double acc = 1.0e-10;    // 1.0×10^-10≒0とし、これを基準に場合分けを行う

    std::vector< complex< double > > test_pole;
    test_pole.reserve( 6 );

    test_pole.emplace_back( -coef.at( 1 ) );
    printf( "%.15f %.15f\n", -coef.at( 1 ), 0.0 );

    for ( unsigned int m = 2; m < 8; m += 2 )
    {
        double disc = coef.at( m ) * coef.at( m ) - 4.0 * coef.at( m + 1 );

        if ( disc < -acc )    // 複素解
        {
            double disc_sqrt = sqrt( -disc );
            complex< double > plus_test_pole( -coef.at( m ) / 2.0, disc_sqrt / 2.0 );
            complex< double > minus_test_pole( -coef.at( m ) / 2.0, -disc_sqrt / 2.0 );
            test_pole.emplace_back( plus_test_pole );
            test_pole.emplace_back( minus_test_pole );
            printf( "%.15f %.15f\n", plus_test_pole.real(), plus_test_pole.imag() );
            printf( "%.15f %.15f\n", minus_test_pole.real(), minus_test_pole.imag() );
        }
        else if ( disc > acc )    // 実数解
        {
            double disc_sqrt = sqrt( disc );
            complex< double > plus_test_pole( ( -coef.at( m ) + disc_sqrt ) / 2.0, 0.0 );
            complex< double > minus_test_pole( ( -coef.at( m ) - disc_sqrt ) / 2.0, 0.0 );
            test_pole.emplace_back( plus_test_pole );
            test_pole.emplace_back( minus_test_pole );
            printf( "%.15f %.15f\n", plus_test_pole.real(), plus_test_pole.imag() );
            printf( "%.15f %.15f\n", minus_test_pole.real(), minus_test_pole.imag() );
        }
        else if ( disc >= -acc && disc <= acc )    // 重解
        {
            complex< double > multiple_test_pole( -coef.at( m ) / 2.0, 0.0 );
            test_pole.emplace_back( multiple_test_pole );
            printf( "%.15f %.15f\n", multiple_test_pole.real(), multiple_test_pole.imag() );
        }
        else
        {
            exit( -1 );    // 場合分けで漏れた場合エラーが発生
        }
    }

    const unsigned int acc2 = 1000;    // 10^-n(小数点以下n桁)まで精度を検査
    for ( unsigned int n = 0; n < 5; n++ )
    {
        assert( static_cast< size_t >( std::round( pole_res.at( n ).real() * acc2 ) ) == static_cast< size_t >( std::round( test_pole.at( n ).real() * acc2 ) ) );
        assert( static_cast< size_t >( std::round( pole_res.at( n ).imag() * acc2 ) ) == static_cast< size_t >( std::round( test_pole.at( n ).imag() * acc2 ) ) );
    }
}

void test_FilterParam_zero_even()
{
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.1, 0.145 );
    FilterParam fparam( 6, 0, bands, 200, 50, 5.0 );

    vector< double > coef {
        -0.040404875,

        -4.097963802039866,         // 複素解の組み合わせ
        5.496940685423355,          // 複素解の組み合わせ
        -1.585891794,               // 実数解の組み合わせ
        -1.903482473,               // 実数解の組み合わせ
        1.1168784833810899,         // 重解の組み合わせ
        0.3118543866599108769856    // 重解の組み合わせ
    };

    auto zero_res = fparam.zero_res( coef );

    for ( auto zero : zero_res )
    {
        printf( "%.15f %.15f\n", zero.real(), zero.imag() );
    }

    const double acc = 1.0e-10;    // 1.0×10^-10≒0とし、これを基準に場合分けを行う

    std::vector< complex< double > > test_zero;
    test_zero.reserve( 5 );

    for ( unsigned int n = 1; n < 7; n += 2 )
    {
        double disc = coef.at( n ) * coef.at( n ) - 4.0 * coef.at( n + 1 );

        if ( disc < -acc )    // 複素解
        {
            double disc_sqrt = sqrt( -disc );
            complex< double > plus_test_zero( -coef.at( n ) / 2.0, disc_sqrt / 2.0 );
            complex< double > minus_test_zero( -coef.at( n ) / 2.0, -disc_sqrt / 2.0 );
            test_zero.emplace_back( plus_test_zero );
            test_zero.emplace_back( minus_test_zero );
            printf( "%.15f %.15f\n", plus_test_zero.real(), plus_test_zero.imag() );
            printf( "%.15f %.15f\n", minus_test_zero.real(), minus_test_zero.imag() );
        }
        else if ( disc > acc )    // 実数解
        {
            double disc_sqrt = sqrt( disc );
            complex< double > plus_test_zero( ( -coef.at( n ) + disc_sqrt ) / 2.0, 0.0 );
            complex< double > minus_test_zero( ( -coef.at( n ) - disc_sqrt ) / 2.0, 0.0 );
            test_zero.emplace_back( plus_test_zero );
            test_zero.emplace_back( minus_test_zero );
            printf( "%.15f %.15f\n", plus_test_zero.real(), plus_test_zero.imag() );
            printf( "%.15f %.15f\n", minus_test_zero.real(), minus_test_zero.imag() );
        }
        else if ( disc >= -acc && disc <= acc )    // 重解
        {
            complex< double > multiple_test_zero( -coef.at( n ) / 2.0, 0.0 );
            test_zero.emplace_back( multiple_test_zero );
            printf( "%.15f %.15f\n", multiple_test_zero.real(), multiple_test_zero.imag() );
        }
        else
        {
            exit( -1 );    // 場合分けで漏れた場合エラーが発生
        }
    }

    const unsigned int acc2 = 1000;    // 10^-n(小数点以下n桁)まで精度を検査
    for ( unsigned int m = 0; m < 5; m++ )
    {
        assert( static_cast< size_t >( std::round( zero_res.at( m ).real() * acc2 ) ) == static_cast< size_t >( std::round( test_zero.at( m ).real() * acc2 ) ) );
        assert( static_cast< size_t >( std::round( zero_res.at( m ).imag() * acc2 ) ) == static_cast< size_t >( std::round( test_zero.at( m ).imag() * acc2 ) ) );
    }
}

void test_FilterParam_zero_odd()
{
    auto bands = FilterParam::gen_bands( FilterType::LPF, 0.2, 0.275 );
    FilterParam fparam( 7, 0, bands, 200, 50, 5.0 );

    vector< double > coef {
        0.025247504683641238,

        0.8885952985540255,
        -4.097963802039866,         // 複素解の組み合わせ
        5.496940685423355,          // 複素解の組み合わせ
        -1.585891794,               // 実数解の組み合わせ
        -1.903482473,               // 実数解の組み合わせ
        1.1168784833810899,         // 重解の組み合わせ
        0.3118543866599108769856    // 重解の組み合わせ
    };

    auto zero_res = fparam.zero_res( coef );

    for ( auto zero : zero_res )
    {
        printf( "%.15f %.15f\n", zero.real(), zero.imag() );
    }

    const double acc = 1.0e-10;    // 1.0×10^-10≒0とし、これを基準に場合分けを行う

    std::vector< complex< double > > test_zero;
    test_zero.reserve( 6 );

    test_zero.emplace_back( -coef.at( 1 ) );
    printf( "%.15f %.15f\n", -coef.at( 1 ), 0.0 );

    for ( unsigned int n = 2; n < 8; n += 2 )
    {
        double disc = coef.at( n ) * coef.at( n ) - 4.0 * coef.at( n + 1 );

        if ( disc < -acc )    // 複素解
        {
            double disc_sqrt = sqrt( -disc );
            complex< double > plus_test_zero( -coef.at( n ) / 2.0, disc_sqrt / 2.0 );
            complex< double > minus_test_zero( -coef.at( n ) / 2.0, -disc_sqrt / 2.0 );
            test_zero.emplace_back( plus_test_zero );
            test_zero.emplace_back( minus_test_zero );
            printf( "%.15f %.15f\n", plus_test_zero.real(), plus_test_zero.imag() );
            printf( "%.15f %.15f\n", minus_test_zero.real(), minus_test_zero.imag() );
        }
        else if ( disc > acc )    // 実数解
        {
            double disc_sqrt = sqrt( disc );
            complex< double > plus_test_zero( ( -coef.at( n ) + disc_sqrt ) / 2.0, 0.0 );
            complex< double > minus_test_zero( ( -coef.at( n ) - disc_sqrt ) / 2.0, 0.0 );
            test_zero.emplace_back( plus_test_zero );
            test_zero.emplace_back( minus_test_zero );
            printf( "%.15f %.15f\n", plus_test_zero.real(), plus_test_zero.imag() );
            printf( "%.15f %.15f\n", minus_test_zero.real(), minus_test_zero.imag() );
        }
        else if ( disc >= -acc && disc <= acc )    // 重解
        {
            complex< double > multiple_test_zero( -coef.at( n ) / 2.0, 0.0 );
            test_zero.emplace_back( multiple_test_zero );
            printf( "%.15f %.15f\n", multiple_test_zero.real(), multiple_test_zero.imag() );
        }
        else
        {
            exit( -1 );    // 場合分けで漏れた場合エラーが発生
        }
    }

    const unsigned int acc2 = 1000;    // 10^-n(小数点以下n桁)まで精度を検査
    for ( unsigned int m = 0; m < 5; m++ )
    {
        assert( static_cast< size_t >( std::round( zero_res.at( m ).real() * acc2 ) ) == static_cast< size_t >( std::round( test_zero.at( m ).real() * acc2 ) ) );
        assert( static_cast< size_t >( std::round( zero_res.at( m ).imag() * acc2 ) ) == static_cast< size_t >( std::round( test_zero.at( m ).imag() * acc2 ) ) );
    }
}
