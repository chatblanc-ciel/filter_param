/*
 * filter_param.cpp
 *
 *  Created on: 2021/07/22
 *      Author: matsu
 */

#include "cascade_iir.hpp"


namespace filter
{

    FILE* fileopen(
        const std::string& filename,
        const char mode,
        const std::string& call_file,
        const int call_line )
    {
        FILE* fp = fopen( filename.c_str(), &mode );
        if ( fp == NULL )
        {
            fprintf(
                stderr,
                "Error: [%s l.%d]Can't open file.(file name : %s, mode : %c)\n",
                call_file.c_str(), call_line, filename.c_str(), mode );
            exit( EXIT_FAILURE );
        }
        return fp;
    }

    std::vector< std::string > split_char( std::string& str, char spliter )
    {
        std::stringstream ss { str };
        std::vector< std::string > strs;
        std::string buf;
        while ( std::getline( ss, buf, spliter ) )
        {
            strs.emplace_back( buf );
        }
        return strs;
    }

    namespace iir
    {

        /* # フィルタ構造体
         *   FilterParamという構造体でメンバ変数を()内で初期化、これらの情報を基に
         *   ・帯域の種類ごとに分割数を算出
         *   ・基本波・第２次高調波の複素正弦波と所望周波数応答の帯域ごとの格納
         *   ・分母分子次数の偶奇の組み合わせによる使用する関数の分岐
         *   を行う
         *   実処理は単一帯域をベクトルに変換して、もう一つのコンストラクタに移譲する
         *
         * # 引数
         * unsigned int zero : 零点の数
         * unsigned int pole : 極の数
         * BandParam input_band :
         * バンドの特性情報(帯域の種類、帯域の左端正規化周波数、帯域の右端正規化周波数)
         * unsigned int input_nsplit_approx : 近似域の分割数
         * unsigned int input_nsplit_transition : 遷移域の分割数
         * double gd : 所望群遅延
         */
        FilterParam::FilterParam(
            unsigned int zero,
            unsigned int pole,
            BandParam input_band,
            unsigned int input_nsplit_approx,
            unsigned int input_nsplit_transition,
            double gd )
            : FilterParam( zero, pole, std::vector< BandParam > { input_band }, input_nsplit_approx, input_nsplit_transition, gd )
        {}

        FilterParam::FilterParam(
            unsigned int zero,
            unsigned int pole,
            std::vector< BandParam > input_bands,
            unsigned int input_nsplit_approx,
            unsigned int input_nsplit_transition,
            double gd )
            : n_order( zero ), m_order( pole ), bands( input_bands ),
              nsplit_approx( input_nsplit_approx ),
              nsplit_transition( input_nsplit_transition ), group_delay( gd ),
              threshold_riple( 1.0 )
        {
            using std::vector;
            const double acc = 1.0e-10;    // 1.0×10^-10≒0

            // 周波数帯域の整合性チェック
            if ( 2 <= this->bands.size() )
            {
                double band_left = 0.0;
                for ( auto bp : bands )
                {
                    if ( std::abs( bp.left() - band_left ) > acc )
                    {
                        fprintf(
                            stderr,
                            "Error: [%s l.%d]Adjacent band edge is necessary "
                            "same.\n"
                            "Or first band of left side must be 0.0.\n",
                            __FILE__, __LINE__ );
                        for ( auto bp_ : bands )
                        {
                            fprintf( stderr, "%s\n", bp_.sprint().c_str() );
                        }
                        exit( EXIT_FAILURE );
                    }
                    band_left = bp.right();
                }
                if ( std::abs( bands.back().right() - 0.5 ) > acc )
                {
                    fprintf(
                        stderr,
                        "Error: [%s l.%d]Last band of right side must be 0.5.\n",
                        __FILE__, __LINE__ );
                    for ( auto bp : bands )
                    {
                        fprintf( stderr, "%s\n", bp.sprint().c_str() );
                    }
                    exit( EXIT_FAILURE );
                }
            }

            // 帯域ごとの分割数算出
            double approx_range = 0.0;
            double transition_range = 0.0;

            for ( auto bp : bands )
            {
                switch ( bp.type() )
                {
                    case BandType::Pass:
                        {
                            approx_range += bp.width();
                            break;
                        }
                    case BandType::Stop:
                        {
                            approx_range += bp.width();
                            break;
                        }
                    case BandType::Transition:
                        {
                            transition_range += bp.width();
                            break;
                        }
                    default:
                        {
                            fprintf(
                                stderr, "Error: [%s l.%d]Undefined band.\n",
                                __FILE__, __LINE__ );
                            exit( EXIT_FAILURE );
                        }
                }
            }

            vector< unsigned int > split;
            split.reserve( bands.size() );
            for ( auto bp : bands )
            {
                switch ( bp.type() )
                {
                    case BandType::Pass:
                        {
                            split.emplace_back( static_cast< unsigned int >(
                                ( static_cast< double >( nsplit_approx )
                                  * bp.width() / approx_range ) ) );
                            break;
                        }
                    case BandType::Stop:
                        {
                            split.emplace_back( static_cast< unsigned int >(
                                ( static_cast< double >( nsplit_approx )
                                  * bp.width() / approx_range ) ) );
                            break;
                        }
                    case BandType::Transition:
                        {
                            split.emplace_back( static_cast< unsigned int >(
                                ( static_cast< double >( nsplit_transition )
                                  * bp.width() / transition_range ) ) );
                            break;
                        }
                    default:
                        {
                            fprintf(
                                stderr, "Error: [%s l.%d]Undefined band.\n",
                                __FILE__, __LINE__ );
                            exit( EXIT_FAILURE );
                        }
                }
            }
            split.at( 0 ) += 1;

            // generate complex sin wave(e^-jω)
            // desire frequency response
            csw.reserve( bands.size() );
            csw2.reserve( bands.size() );
            desire_res.reserve( bands.size() );
            for ( unsigned int i = 0; i < bands.size(); ++i )
            {
                csw.emplace_back( gen_csw( bands.at( i ), split.at( i ) ) );
                csw2.emplace_back( gen_csw2( bands.at( i ), split.at( i ) ) );
                desire_res.emplace_back( gen_desire_res(
                    bands.at( i ), split.at( i ), group_delay ) );
            }

            // decide using function
            if ( ( n_order % 2 ) == 0 )
            {
                if ( ( m_order % 2 ) == 0 )
                {
                    this->freq_res_func = &FilterParam::freq_res_se;
                    this->group_delay_func = &FilterParam::group_delay_se;
                    this->stability_func = &FilterParam::judge_stability_even;
                    this->pole_func = &FilterParam::pole_even;
                    this->zero_func = &FilterParam::zero_even;
                }
                else
                {
                    this->freq_res_func = &FilterParam::freq_res_mo;
                    this->group_delay_func = &FilterParam::group_delay_mo;
                    this->stability_func = &FilterParam::judge_stability_odd;
                    this->pole_func = &FilterParam::pole_even;
                    this->zero_func = &FilterParam::zero_odd;
                }
            }
            else
            {
                if ( ( m_order % 2 ) == 0 )
                {
                    this->freq_res_func = &FilterParam::freq_res_no;
                    this->group_delay_func = &FilterParam::group_delay_no;
                    this->stability_func = &FilterParam::judge_stability_even;
                    this->pole_func = &FilterParam::pole_odd;
                    this->zero_func = &FilterParam::zero_even;
                }
                else
                {
                    this->freq_res_func = &FilterParam::freq_res_so;
                    this->group_delay_func = &FilterParam::group_delay_so;
                    this->stability_func = &FilterParam::judge_stability_odd;
                    this->pole_func = &FilterParam::pole_odd;
                    this->zero_func = &FilterParam::zero_odd;
                }
            }
        }

        /* # フィルタ構造体
         *   CSVファイルから所望特性を読み取る関数
         *   複数の所望特性を読み取り，フィルタ構造体の配列で返却
         *   CSVファイルの書式は以下の通り
         *
         *     No, Numerator, Denominator, State, GroupDelay, NsplitApprox,
         * NsplitTransition No : 所望特性のナンバリング Numerator : 分子次数
         *         Denominator : 分母次数
         *         State : フィルタの特性情報(L.P.F.など)
         *         GroupDelay : 所望群遅延
         *         NsplitApprox : 近似帯域の分割数
         *         NsplitTransition : 遷移域の分割数
         *
         *   vectorのインデックス = Noなので
         *   "No"は読みこまない
         *   また，１行目はヘッダなので読み飛ばす
         *
         * # 引数
         * string& filename : CSVファイルのパス(exeからの相対パスでも可能)
         * # 返り値
         * vector<FilterParam> params :
         * CSVファイルにある分，全部の所望特性を格納した配列
         */
        std::vector< FilterParam >
        FilterParam::read_csv( std::string& filename )
        {
            using std::vector;

            vector< FilterParam > filter_params;
            std::ifstream ifs( filename );
            if ( !ifs )
            {
                fprintf(
                    stderr,
                    "Error: [%s l.%d]Can't open file.(file name : %s, mode : "
                    "read)\n",
                    __FILE__, __LINE__, filename.c_str() );
                exit( EXIT_FAILURE );
            }

            std::string buf;
            getline( ifs, buf );    // ヘッダ読み飛ばし
            while ( getline( ifs, buf ) )
            {
                auto vals = split_char( buf, ',' );
                auto type = FilterParam::analyze_type( vals.at( 3 ) );
                vector< BandParam > bands;

                switch ( type )
                {
                    case FilterType::LPF:
                        {
                            auto edges =
                                FilterParam::analyze_edges( vals.at( 3 ) );
                            if ( edges.size() != 2 )
                            {
                                fprintf(
                                    stderr,
                                    "Error: [%s l.%d]Format of filter state is "
                                    "illegal.(input : \"%s\")\n"
                                    "If you assign filter type L.P.F. , length "
                                    "of edges is only 2.\n",
                                    __FILE__, __LINE__, vals.at( 3 ).c_str() );
                                exit( EXIT_FAILURE );
                            }
                            bands = FilterParam::gen_bands(
                                FilterType::LPF, edges.at( 0 ), edges.at( 1 ) );
                            break;
                        }
                    case FilterType::HPF:
                    case FilterType::BPF:
                    case FilterType::BEF:
                    case FilterType::Other:
                    default:
                        {
                            fprintf(
                                stderr,
                                "Error: [%s l.%d]It has not been implement "
                                "yet.(input : \"%s\")\n",
                                __FILE__, __LINE__, vals.at( 3 ).c_str() );
                            exit( EXIT_FAILURE );
                        }
                }

                filter_params.emplace_back( FilterParam(
                    static_cast< unsigned int >( atoi( vals.at( 1 ).c_str() ) ),
                    static_cast< unsigned int >( atoi( vals.at( 2 ).c_str() ) ),
                    bands,
                    static_cast< unsigned int >(
                        ( atoi( vals.at( 5 ).c_str() ) ) ),
                    static_cast< unsigned int >(
                        ( atoi( vals.at( 6 ).c_str() ) ) ),
                    atof( vals.at( 4 ).c_str() ) ) );
            }

            return filter_params;
        }

        /* フィルタのタイプを簡易入力(文字列)する
         *   LPF(0.2, 0.3) : L.P.F.で通過域端 0.2, 阻止域端 0.3のフィルタ
         *
         *   Other(path) :
         * その他の特性のフィルタ。pathは周波数帯域の情報を記したファイルへの相対パス
         */
        FilterType FilterParam::analyze_type( const std::string& input )
        {
            using std::string;

            FilterType type;
            string str = input;
            str.erase( remove( str.begin(), str.end(), ' ' ), str.end() );

#if __GNUC__ > 5
            string csv_space =
                regex_replace( str, std::regex( "[(),:]+" ), string( " " ) );
#else
            string csv_space = std::move( str );
            for ( auto& c : csv_space )
            {
                if ( c == '(' || c == ')' || c == ',' || c == ':' )
                    c = ' ';
            }
#endif

            auto input_type = split_char( csv_space, ' ' );

            if ( input_type.size() <= 1 )
            {
                fprintf(
                    stderr,
                    "Error: [%s l.%d]Format of filter type is illegal.(input : "
                    "\"%s\")\n",
                    __FILE__, __LINE__, input.c_str() );
                exit( EXIT_FAILURE );
            }
            else if ( input_type.at( 0 ) == "LPF" )
            {
                type = FilterType::LPF;
            }
            else if ( true )    // hpf, bpf, bef バリエーション
            {
                fprintf(
                    stderr,
                    "Error: [%s l.%d]It has not been implement yet.(input : "
                    "\"%s\")\n",
                    __FILE__, __LINE__, input.c_str() );
                exit( EXIT_FAILURE );
            }
            else
            {
                fprintf(
                    stderr,
                    "Error: [%s l.%d]Filter Type is undefined.(input : "
                    "\"%s\")\n",
                    __FILE__, __LINE__, input.c_str() );
                exit( EXIT_FAILURE );
            }

            return type;
        }

        /* # フィルタ構造体
         *   フィルタの周波数帯域端を文字列から分離する
         *   LPF(0.2, 0.3) : 帯域域端 0.2, 帯域端 0.3
         *
         * # 引数
         * string& input : 解析する文字列
         * # 返り値
         * vector<double> edge : 解析・分離した周波数帯域端の配列
         */
        std::vector< double >
        FilterParam::analyze_edges( const std::string& input )
        {
            using std::string;
            using std::vector;

            vector< double > edge;
            string str = input;
            str.erase( remove( str.begin(), str.end(), ' ' ), str.end() );

#if __GNUC__ > 5
            string csv_space =
                regex_replace( str, std::regex( "[(),:]+" ), string( " " ) );
#else
            string csv_space = std::move( str );
            for ( auto& c : csv_space )
            {
                if ( c == '(' || c == ')' || c == ',' || c == ':' )
                    c = ' ';
            }
#endif

            auto input_type = split_char( csv_space, ' ' );
            input_type.erase( input_type.begin() );

            if ( input_type.size() <= 0 )
            {
                fprintf(
                    stderr,
                    "Error: [%s l.%d]Format of filter state is illegal.(input "
                    ": \"%s\")\n",
                    __FILE__, __LINE__, input.c_str() );
                exit( EXIT_FAILURE );
            }
            for ( auto v : input_type )
            {
                edge.emplace_back( atof( v.c_str() ) );
            }
            return edge;
        }

        /* # フィルタ構造体
         *   複素正弦波の基本波(e^-jω)を生成する関数
         *   刻みは引数の周波数帯域幅と分割数に応じる
         *
         * # 引数
         * BandParam& bp : 周波数帯域情報をもった構造体
         * unsigned int nsplit : 周波数帯域の分割数
         * # 返り値
         * vector<complex<double>> csw : 引数の周波数帯域幅と分割数に応じた
         *                                        複素正弦波の配列
         *                                        csw = Complex Sin Wave
         *
         */
        std::vector< std::complex< double > >
        FilterParam::gen_csw( const BandParam& bp, const unsigned int nsplit )
        {
            std::vector< std::complex< double > > csw;
            csw.reserve( nsplit );
            const double step_size =
                bp.width() / static_cast< double >( nsplit );
            const double left = bp.left();
            constexpr double dpi = -2.0 * M_PI;    // double pi

            for ( unsigned int i = 0; i < nsplit; ++i )
            {
                csw.emplace_back( std::polar(
                    1.0,
                    dpi * ( left + step_size * static_cast< double >( i ) ) ) );
            }

            return csw;
        }

        /* # フィルタ構造体
         *   複素正弦波の第２次高調波(e^-j2ω)を生成する関数
         *   刻みは引数の周波数帯域幅と分割数に応じる
         *
         * # 引数
         * BandParam& bp : 周波数帯域情報をもった構造体
         * unsigned int nsplit : 周波数帯域の分割数
         * # 返り値
         * vector<complex<double>> csw2 : 引数の周波数帯域幅と分割数に応じた
         *                                        複素正弦波の配列
         *                                        csw2 = Complex Sin Wave
         * 2(second)
         *
         */
        std::vector< std::complex< double > >
        FilterParam::gen_csw2( const BandParam& bp, const unsigned int nsplit )
        {
            std::vector< std::complex< double > > csw2;
            csw2.reserve( nsplit );
            const double step_size =
                bp.width() / static_cast< double >( nsplit );
            const double left = bp.left();
            constexpr double dpi = -4.0 * M_PI;    // quadrical pi

            for ( unsigned int i = 0; i < nsplit; ++i )
            {
                csw2.emplace_back( std::polar(
                    1.0,
                    dpi * ( left + step_size * static_cast< double >( i ) ) ) );
            }

            return csw2;
        }

        std::vector< std::complex< double > > FilterParam::gen_desire_res(
            const BandParam& bp,
            const unsigned int nsplit,
            const double group_delay )
        {
            std::vector< std::complex< double > > desire;

            switch ( bp.type() )
            {
                case BandType::Pass:
                    {
                        desire.reserve( nsplit );
                        const double step_size =
                            bp.width() / static_cast< double >( nsplit );
                        const double left = bp.left();
                        constexpr double dpi = 2.0 * M_PI;    // double pi
                        const double ang = -dpi * group_delay;

                        for ( unsigned int i = 0; i < nsplit; ++i )
                        {
                            desire.emplace_back( std::polar(
                                1.0, ang
                                         * ( left
                                             + step_size
                                                   * static_cast< double >(
                                                       i ) ) ) );
                        }
                        break;
                    }
                case BandType::Stop:
                    {
                        desire.resize( nsplit, 0.0 );
                        break;
                    }
                case BandType::Transition: break;
                default:
                    {
                        fprintf(
                            stderr, "Error: [%s l.%d]Undefined band.\n",
                            __FILE__, __LINE__ );
                        exit( EXIT_FAILURE );
                    }
            }

            return desire;
        }

        std::vector< std::vector< std::complex< double > > >
        FilterParam::freq_res_se( const std::vector< double >& coef ) const
        {
            using std::complex;
            using std::vector;

            vector< vector< complex< double > > > res;
            res.reserve( bands.size() );

            for ( unsigned int i = 0; i < bands.size();
                  ++i )    // 周波数帯域のループ
            {
                vector< complex< double > > band_res;
                band_res.reserve( csw.at( i ).size() );

                for ( unsigned int j = 0; j < csw.at( i ).size();
                      ++j )    // 周波数帯域内の分割数によるループ
                {
                    complex< double > frac_over( 1.0, 1.0 );
                    complex< double > frac_under( 1.0, 1.0 );

                    for ( unsigned int n = 1; n < n_order; n += 2 )
                    {
                        frac_over *= 1.0 + coef.at( n ) * csw.at( i ).at( j )
                                     + coef.at( n + 1 ) * csw2.at( i ).at( j );
                    }
                    for ( unsigned int m = n_order + 1; m < opt_order();
                          m += 2 )
                    {
                        frac_under *= 1.0 + coef.at( m ) * csw.at( i ).at( j )
                                      + coef.at( m + 1 ) * csw2.at( i ).at( j );
                    }
                    band_res.emplace_back(
                        coef.at( 0 ) * ( frac_over / frac_under ) );
                }
                res.emplace_back( band_res );
            }

            return res;
        }

        std::vector< std::vector< std::complex< double > > >
        FilterParam::freq_res_so( const std::vector< double >& coef ) const
        {
            using std::complex;
            using std::vector;

            vector< vector< complex< double > > > res;
            res.reserve( bands.size() );

            for ( unsigned int i = 0; i < bands.size();
                  ++i )    // 周波数帯域のループ
            {
                vector< complex< double > > band_res;
                band_res.reserve( csw.at( i ).size() );

                for ( unsigned int j = 0; j < csw.at( i ).size();
                      ++j )    // 周波数帯域内の分割数によるループ
                {
                    complex< double > frac_over( 1.0, 1.0 );
                    complex< double > frac_under( 1.0, 1.0 );

                    frac_over *= 1.0 + coef.at( 1 ) * csw.at( i ).at( j );
                    for ( unsigned int n = 2; n < n_order; n += 2 )
                    {
                        frac_over *= 1.0 + coef.at( n ) * csw.at( i ).at( j )
                                     + coef.at( n + 1 ) * csw2.at( i ).at( j );
                    }

                    frac_under *=
                        1.0 + coef.at( n_order + 1 ) * csw.at( i ).at( j );
                    for ( unsigned int m = n_order + 2; m < opt_order();
                          m += 2 )
                    {
                        frac_under *= 1.0 + coef.at( m ) * csw.at( i ).at( j )
                                      + coef.at( m + 1 ) * csw2.at( i ).at( j );
                    }

                    band_res.emplace_back(
                        coef.at( 0 ) * ( frac_over / frac_under ) );
                }
                res.emplace_back( band_res );
            }
            return res;
        }

        std::vector< std::vector< std::complex< double > > >
        FilterParam::freq_res_no( const std::vector< double >& coef ) const
        {
            using std::complex;
            using std::vector;

            vector< vector< complex< double > > > freq;
            freq.reserve( bands.size() );

            for ( unsigned int i = 0; i < bands.size();
                  ++i )    // 周波数帯域のループ
            {
                vector< complex< double > > freq_band;
                freq_band.reserve( csw.at( i ).size() );

                for ( unsigned int j = 0; j < csw.at( i ).size();
                      ++j )    // 周波数帯域内の分割数によるループ
                {
                    complex< double > freq_denominator( 1.0, 1.0 );
                    complex< double > freq_numerator( 1.0, 1.0 );

                    freq_numerator *= 1.0 + coef.at( 1 ) * csw.at( i ).at( j );
                    for ( unsigned int n = 2; n < n_order;
                          n += 2 )    //分子の総乗ループ
                    {
                        freq_numerator *=
                            1.0 + coef.at( n ) * csw.at( i ).at( j )
                            + coef.at( n + 1 ) * csw2.at( i ).at( j );
                    }
                    for ( unsigned int m = n_order + 1; m < opt_order();
                          m += 2 )    //分母の総乗ループ
                    {
                        freq_denominator *=
                            1.0 + coef.at( m ) * csw.at( i ).at( j )
                            + coef.at( m + 1 ) * csw2.at( i ).at( j );
                    }

                    freq_band.emplace_back(
                        coef.at( 0 ) * ( freq_numerator / freq_denominator ) );
                }
                freq.emplace_back( freq_band );
            }

            return freq;
        }

        std::vector< std::vector< std::complex< double > > >
        FilterParam::freq_res_mo( const std::vector< double >& coef ) const
        {
            using std::complex;
            using std::vector;

            vector< vector< complex< double > > > freq;
            freq.reserve( bands.size() );

            for ( unsigned int i = 0; i < bands.size();
                  ++i )    // 周波数帯域のループ
            {
                vector< complex< double > > freq_band;
                freq_band.reserve( csw.at( i ).size() );

                for ( unsigned int j = 0; j < csw.at( i ).size();
                      ++j )    // 周波数帯域内の分割数によるループ
                {
                    complex< double > nume( 1.0, 1.0 );
                    complex< double > deno( 1.0, 1.0 );

                    for ( unsigned int n = 1; n < n_order; n += 2 )
                    {
                        nume *= 1.0 + coef.at( n ) * csw.at( i ).at( j )
                                + coef.at( n + 1 ) * csw2.at( i ).at( j );
                    }
                    deno *= 1.0 + coef.at( n_order + 1 ) * csw.at( i ).at( j );
                    for ( unsigned int m = n_order + 2; m < opt_order();
                          m += 2 )
                    {
                        deno *= 1.0 + coef.at( m ) * csw.at( i ).at( j )
                                + coef.at( m + 1 ) * csw2.at( i ).at( j );
                    }
                    freq_band.emplace_back( coef.at( 0 ) * ( nume / deno ) );
                }
                freq.emplace_back( freq_band );
            }
            return freq;
        }

        std::vector< std::vector< double > >
        FilterParam::group_delay_se( const std::vector< double >& coef ) const
        {
            using std::complex;
            using std::vector;

            vector< vector< double > > res;
            res.reserve( bands.size() );

            for ( unsigned int i = 0; i < bands.size();
                  ++i )    // 周波数帯域のループ
            {
                vector< double > band_res;
                band_res.reserve( csw.at( i ).size() );

                for ( unsigned int j = 0; j < csw.at( i ).size();
                      ++j )    // 周波数帯域内の分割数によるループ
                {
                    complex< double > second_over( 0.0, 0.0 );
                    complex< double > second_under( 0.0, 0.0 );

                    for ( unsigned int n = 1; n < n_order; n += 2 )
                    {
                        second_over +=
                            ( coef.at( n ) * csw.at( i ).at( j )
                              + 2.0 * coef.at( n + 1 ) * csw2.at( i ).at( j ) )
                            / ( 1.0 + coef.at( n ) * csw.at( i ).at( j )
                                + coef.at( n + 1 ) * csw2.at( i ).at( j ) );
                    }
                    for ( unsigned int m = n_order + 1; m < opt_order();
                          m += 2 )
                    {
                        second_under +=
                            ( coef.at( m ) * csw.at( i ).at( j )
                              + 2.0 * coef.at( m + 1 ) * csw2.at( i ).at( j ) )
                            / ( 1.0 + coef.at( m ) * csw.at( i ).at( j )
                                + coef.at( m + 1 ) * csw2.at( i ).at( j ) );
                    }
                    complex< double > second_gd = second_over - second_under;

                    band_res.emplace_back( second_gd.real() );
                }
                res.emplace_back( band_res );
            }
            return res;
        }

        std::vector< std::vector< double > >
        FilterParam::group_delay_so( const std::vector< double >& coef ) const
        {
            using std::complex;
            using std::vector;

            vector< vector< double > > res;
            res.reserve( bands.size() );

            for ( unsigned int i = 0; i < bands.size();
                  ++i )    // 周波数帯域のループ
            {
                vector< double > band_res;
                band_res.reserve( csw.at( i ).size() );

                for ( unsigned int j = 0; j < csw.at( i ).size();
                      ++j )    // 周波数帯域内の分割数によるループ
                {
                    complex< double > prime_over =
                        ( 1.0 + coef.at( 1 ) * csw.at( i ).at( j ) )
                        / ( coef.at( 1 ) * csw.at( i ).at( j ) );
                    complex< double > prime_under =
                        ( 1.0 + coef.at( n_order + 1 ) * csw.at( i ).at( j ) )
                        / ( coef.at( n_order + 1 ) * csw.at( i ).at( j ) );
                    complex< double > prime_gd = prime_over - prime_under;

                    complex< double > second_over( 0.0, 0.0 );
                    complex< double > second_under( 0.0, 0.0 );

                    for ( unsigned int n = 2; n < n_order; n += 2 )
                    {
                        second_over +=
                            ( coef.at( n ) * csw.at( i ).at( j )
                              + 2.0 * coef.at( n + 1 ) * csw2.at( i ).at( j ) )
                            / ( 1.0 + coef.at( n ) * csw.at( i ).at( j )
                                + coef.at( n + 1 ) * csw2.at( i ).at( j ) );
                    }
                    for ( unsigned int m = n_order + 2; m < opt_order();
                          m += 2 )
                    {
                        second_under +=
                            ( coef.at( m ) * csw.at( i ).at( j )
                              + 2.0 * coef.at( m + 1 ) * csw2.at( i ).at( j ) )
                            / ( 1.0 + coef.at( m ) * csw.at( i ).at( j )
                                + coef.at( m + 1 ) * csw2.at( i ).at( j ) );
                    }
                    complex< double > second_gd = second_over - second_under;

                    band_res.emplace_back( ( prime_gd + second_gd ).real() );
                }
                res.emplace_back( band_res );
            }
            return res;
        }

        std::vector< std::vector< double > >
        FilterParam::group_delay_no( const std::vector< double >& coef ) const
        {
            using std::complex;
            using std::vector;

            vector< vector< double > > res;
            res.reserve( bands.size() );

            for ( unsigned int i = 0; i < bands.size();
                  ++i )    // 周波数帯域のループ
            {
                vector< double > band_res;
                band_res.reserve( csw.at( i ).size() );

                for ( unsigned int j = 0; j < csw.at( i ).size();
                      ++j )    // 周波数帯域内の分割数によるループ
                {
                    complex< double > prime_gd =
                        ( 1.0 + coef.at( 1 ) * csw.at( i ).at( j ) )
                        / ( coef.at( 1 )
                            * csw.at( i ).at(
                                j ) );    // calculate fractional over

                    complex< double > second_over( 0.0, 0.0 );
                    complex< double > second_under( 0.0, 0.0 );

                    for ( unsigned int n = 2; n < n_order; n += 2 )
                    {
                        second_over +=
                            ( coef.at( n ) * csw.at( i ).at( j )
                              + 2.0 * coef.at( n + 1 ) * csw2.at( i ).at( j ) )
                            / ( 1.0 + coef.at( n ) * csw.at( i ).at( j )
                                + coef.at( n + 1 ) * csw2.at( i ).at( j ) );
                    }
                    for ( unsigned int m = n_order + 1; m < opt_order();
                          m += 2 )
                    {
                        second_under +=
                            ( coef.at( m ) * csw.at( i ).at( j )
                              + 2.0 * coef.at( m + 1 ) * csw2.at( i ).at( j ) )
                            / ( 1.0 + coef.at( m ) * csw.at( i ).at( j )
                                + coef.at( m + 1 ) * csw2.at( i ).at( j ) );
                    }
                    complex< double > second_gd = second_over - second_under;

                    band_res.emplace_back( ( prime_gd + second_gd ).real() );
                }
                res.emplace_back( band_res );
            }
            return res;
        }

        std::vector< std::vector< double > >
        FilterParam::group_delay_mo( const std::vector< double >& coef ) const
        {
            using std::complex;
            using std::vector;

            vector< vector< double > > res;
            res.reserve( bands.size() );

            for ( unsigned int i = 0; i < bands.size();
                  ++i )    // 周波数帯域のループ
            {
                vector< double > band_res;
                band_res.reserve( csw.at( i ).size() );

                for ( unsigned int j = 0; j < csw.at( i ).size();
                      ++j )    // 周波数帯域内の分割数によるループ
                {
                    complex< double > prime_gd =
                        -( 1.0 + coef.at( n_order + 1 ) * csw.at( i ).at( j ) )
                        / ( coef.at( n_order + 1 )
                            * csw.at( i ).at(
                                j ) );    // calculate fractional under

                    complex< double > second_over( 0.0, 0.0 );
                    complex< double > second_under( 0.0, 0.0 );

                    for ( unsigned int n = 1; n < n_order; n += 2 )
                    {
                        second_over +=
                            ( coef.at( n ) * csw.at( i ).at( j )
                              + 2.0 * coef.at( n + 1 ) * csw2.at( i ).at( j ) )
                            / ( 1.0 + coef.at( n ) * csw.at( i ).at( j )
                                + coef.at( n + 1 ) * csw2.at( i ).at( j ) );
                    }
                    for ( unsigned int m = n_order + 2; m < opt_order();
                          m += 2 )
                    {
                        second_under +=
                            ( coef.at( m ) * csw.at( i ).at( j )
                              + 2.0 * coef.at( m + 1 ) * csw2.at( i ).at( j ) )
                            / ( 1.0 + coef.at( m ) * csw.at( i ).at( j )
                                + coef.at( m + 1 ) * csw2.at( i ).at( j ) );
                    }
                    complex< double > second_gd = second_over - second_under;

                    band_res.emplace_back( ( prime_gd + second_gd ).real() );
                }
                res.emplace_back( band_res );
            }
            return res;
        }

        double FilterParam::judge_stability_even(
            const std::vector< double >& coef ) const
        {
            double penalty = 0.0;

            for ( unsigned int n = n_order + 1; n < this->opt_order(); n += 2 )
            {
                if ( std::abs( coef.at( n + 1 ) ) >= 1.0
                     || coef.at( n + 1 ) <= std::abs( coef.at( n ) ) - 1.0 )
                {
                    penalty += coef.at( n ) * coef.at( n )
                               + coef.at( n + 1 ) * coef.at( n + 1 );
                }
            }
            return penalty;
        }

        double FilterParam::judge_stability_odd(
            const std::vector< double >& coef ) const
        {
            double penalty = 0.0;

            if ( std::abs( coef.at( n_order + 1 ) ) >= 1.0 )
            {
                penalty += coef.at( n_order + 1 ) * coef.at( n_order + 1 );
            }
            for ( unsigned int m = n_order + 2; m < opt_order(); m += 2 )
            {
                if ( std::abs( coef.at( m + 1 ) ) >= 1.0
                     || coef.at( m + 1 ) <= std::abs( coef.at( m ) ) - 1.0 )
                {
                    penalty += coef.at( m ) * coef.at( m )
                               + coef.at( m + 1 ) * coef.at( m + 1 );
                }
            }

            return penalty;
        }

        /* # フィルタ構造体
         *   ペナルティ関数法による目的関数値を計算する
         *
         */
        double FilterParam::evaluate( const std::vector< double >& coef ) const
        {
            using std::complex;
            using std::vector;

            constexpr double cs = 100.0;    //安定性のペナルティの重み
            constexpr double ct = 100.0;    //振幅隆起のペナルティの重み

            double max_error = 0.0;    //最大誤差
            double max_riple = 0.0;    //振幅隆起のペナルティの値

            double penalty_stability = judge_stability( coef );
            vector< vector< complex< double > > > freq = freq_res( coef );

            for ( unsigned int i = 0; i < bands.size();
                  ++i )    // 周波数帯域のループ
            {
                for ( unsigned int j = 0; j < csw.at( i ).size();
                      ++j )    // 周波数帯域内の分割数によるループ
                {
                    switch ( bands.at( i ).type() )
                    {
                        case BandType::Pass:
                        case BandType::Stop:
                            {
                                double error = std::abs(
                                    desire_res.at( i ).at( j )
                                    - freq.at( i ).at( j ) );
                                if ( max_error < error )
                                {
                                    max_error = error;
                                }
                                break;
                            }
                        case BandType::Transition:
                            {
                                double current_riple =
                                    std::abs( freq.at( i ).at( j ) );
                                if ( current_riple > threshold_riple
                                     && current_riple > max_riple )
                                {
                                    max_riple = current_riple;
                                }
                                break;
                            }
                        default:
                            {
                                fprintf(
                                    stderr, "Error: [%s l.%d]Undefined band.\n",
                                    __FILE__, __LINE__ );
                                exit( EXIT_FAILURE );
                            }
                    }
                }
            }
            return (
                max_error + ct * max_riple * max_riple
                + cs * penalty_stability );
        }

        std::vector< double > FilterParam::init_coef(
            const double a0, const double a, const double b ) const
        {
            using std::uniform_real_distribution;

            thread_local std::random_device rnd;
            thread_local std::mt19937 mt( rnd() );
            uniform_real_distribution<> a0_range(
                -std::abs( a0 ), std::abs( a0 ) );
            uniform_real_distribution<> a_range(
                -std::abs( a ), std::abs( a ) );
            uniform_real_distribution<> b_range(
                -std::abs( b ), std::abs( b ) );

            std::vector< double > coef;
            coef.reserve( this->opt_order() );

            coef.emplace_back( a0_range( mt ) );
            for ( unsigned int n = 0; n < n_order; ++n )
            {
                coef.emplace_back( a_range( mt ) );
            }
            for ( unsigned int m = 0; m < m_order; ++m )
            {
                coef.emplace_back( b_range( mt ) );
            }

            return coef;
        }

        std::vector< double >
        FilterParam::init_stable_coef( const double a0, const double a ) const
        {
            using std::uniform_real_distribution;

            thread_local std::random_device rnd;
            thread_local std::mt19937 mt( rnd() );
            uniform_real_distribution<> a0_range(
                -std::abs( a0 ), std::abs( a0 ) );
            uniform_real_distribution<> a_range(
                -std::abs( a ), std::abs( a ) );
            uniform_real_distribution<> uniform(
                -1.0 + std::numeric_limits< double >::epsilon(), 1.0 );

            std::vector< double > coef;
            coef.reserve( this->opt_order() );

            coef.emplace_back( a0_range( mt ) );
            for ( unsigned int n = 0; n < n_order; ++n )
            {
                coef.emplace_back( a_range( mt ) );
            }
            if ( ( m_order % 2 ) == 1 )
            {
                coef.emplace_back( uniform( mt ) );
            }
            for ( unsigned int m = m_order % 2; m < m_order; m += 2 )
            {
                double b2 = uniform( mt );
                uniform_real_distribution<> b1_range(
                    -( b2 + 1.0 ) + std::numeric_limits< double >::epsilon(),
                    b2 + 1.0 );
                double b1 = b1_range( mt );

                coef.emplace_back( b1 );
                coef.emplace_back( b2 );
            }

            return coef;
        }

        /* # フィルタ構造体
         *   振幅特性図の描画
         * 	 leftとrightで描画範囲の指定[0:0.5]
         *
         * # 引数
         * vector<double> &coef : 係数列
         * string &filename : 出力するファイル名(.png)
         * double left : 帯域の左端正規化周波数 [0:0.5)
         * double right : 帯域の右端正規化周波数 (0:0.5]
         *
         */
        void FilterParam::gprint_amp(
            const std::vector< double >& coef,
            const std::string& filename,
            const double left,
            const double right ) const
        {
            BandParam range( BandType::Pass, left, right );
            FilterParam fparam( n_order, m_order, range, 1000, 0, 5.0 );
            auto freq_res = fparam.freq_res( coef );

            constexpr double dpi = 2.0 * M_PI / 1000.0;    //固定値の事前計算
            const double x_step = ( right - left ) * dpi;

            // gnuplotで出力
            FILE* gp = popen( "gnuplot -persist", "w" );
            fprintf( gp, "set terminal pngcairo size 1280, 960\n" );
            fprintf( gp, "set output '%s'\n", filename.c_str() );
            fprintf( gp, "set grid\n" );
            fprintf(
                gp,
                "set xlabel 'Normalized angular frequency'\n" );    //正規化角周波数
            fprintf( gp, "set ylabel 'Amplitude'\n" );
            fprintf( gp, "set key    font 'Times New Roman,15'\n" );
            fprintf( gp, "set xlabel font 'Times New Roman,20'\n" );
            fprintf( gp, "set ylabel font 'Times New Roman,20'\n" );
            fprintf( gp, "set tics   font 'Times New Roman,15'\n" );
            fprintf(
                gp, "set xrange [%f:%f]\n", left * 2.0 * M_PI,
                right * 2.0 * M_PI );
            fprintf( gp, "set xtics 0, 0.1*pi, pi\n" );
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
            fprintf( gp, "set format x '%.1Pπ'\n" );
#pragma GCC diagnostic pop
            fprintf( gp, "set lmargin 20\n" );
            fprintf( gp, "set bmargin 10\n" );
            fprintf( gp, "plot '-' with lines title \"\n" );

            for ( auto band_res : freq_res )
            {
                for ( unsigned int i = 0; i < band_res.size(); i++ )
                {
                    fprintf(
                        gp, "%f %f\n", left * 2.0 * M_PI + i * x_step,
                        abs( band_res.at( i ) ) );
                }
            }

            fprintf( gp, "e\n" );

            pclose( gp );
        }

        void FilterParam::gprint_mag(
            const std::vector< double >& coef,
            const std::string& filename,
            const double left,
            const double right ) const
        {
            BandParam range( BandType::Pass, left, right );
            FilterParam fparam( n_order, m_order, range, 1000, 0, 5.0 );
            auto freq_res = fparam.freq_res( coef );

            constexpr double dpi = 2.0 * M_PI / 1000.0;    //固定値の事前計算
            const double x_step = ( right - left ) * dpi;

            // gnuplotで出力
            FILE* gp = popen( "gnuplot -persist", "w" );
            fprintf( gp, "set terminal pngcairo size 1280, 960\n" );
            fprintf( gp, "set output '%s'\n", filename.c_str() );
            fprintf( gp, "set grid\n" );
            fprintf(
                gp,
                "set xlabel 'Normalized angular frequency'\n" );    //正規化角周波数
            fprintf( gp, "set ylabel 'Magnitude[dB]'\n" );
            fprintf( gp, "set key    font 'Times New Roman,15'\n" );
            fprintf( gp, "set xlabel font 'Times New Roman,20'\n" );
            fprintf( gp, "set ylabel font 'Times New Roman,20'\n" );
            fprintf( gp, "set tics   font 'Times New Roman,15'\n" );
            fprintf(
                gp, "set xrange [%f:%f]\n", left * 2.0 * M_PI,
                right * 2.0 * M_PI );
            fprintf( gp, "set xtics 0, 0.1*pi, pi\n" );
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
            fprintf( gp, "set format x '%.1Pπ'\n" );
#pragma GCC diagnostic pop
            fprintf( gp, "set lmargin 20\n" );
            fprintf( gp, "set bmargin 10\n" );
            fprintf( gp, "plot '-' with lines title \"\n" );

            for ( auto band_res : freq_res )
            {
                for ( unsigned int i = 0; i < band_res.size(); i++ )
                {
                    fprintf(
                        gp, "%f %f\n", left * 2.0 * M_PI + i * x_step,
                        20 * log10( abs( band_res.at( i ) ) ) );
                }
            }

            fprintf( gp, "e\n" );

            pclose( gp );
        }


        /* # フィルタ構造体
         *   分母の係数列から極、分子の係数列から零点の計算を係数列数が奇数の場合と偶数の場合で行う
         *   さらに判別式の値から複素解・実数解・重解の予想される出現頻度順に場合分けを行う
         *
         * # 引数
         * vector<double>& coef : 係数列
         * # 返り値
         * vector<complex<double>> pole : 極の計算結果の配列
         * vector<complex<double>> zero : 零点の計算結果の配列
         *
         */
        std::vector< std::complex< double > > FilterParam::pole_even( const std::vector< double >& coef ) const
        {
            using std::abs;    // c++でdouble型でも使用可能
            using std::complex;
            using std::sqrt;

            constexpr double acc = 1.0e-10;    // 1.0×10^-10≒0とし、これを基準に場合分けを行う

            std::vector< complex< double > > pole;
            pole.reserve( 2 * m_order );

            for ( unsigned int m = n_order + 1; m < opt_order(); m += 2 )
            {
                double disc = coef.at( m ) * coef.at( m ) - 4.0 * coef.at( m + 1 );

                if ( disc < -acc )    // 複素解
                {
                    pole.emplace_back( complex< double >( -0.5 * coef.at( m ), 0.5 * sqrt( abs( disc ) ) ) );    // plus_pole
                    pole.emplace_back( conj( pole.back() ) );                                                    // minus_pole
                }
                else if ( disc > acc )    // 実数解
                {
                    double disc_sqrt = sqrt( disc );
                    pole.emplace_back( complex< double >( 0.5 * ( -coef.at( m ) + disc_sqrt ), 0.0 ) );    // plus_pole
                    pole.emplace_back( complex< double >( 0.5 * ( -coef.at( m ) - disc_sqrt ), 0.0 ) );    // minus_pole
                }
                else    // 重解
                {
                    pole.emplace_back( complex< double >( -0.5 * coef.at( m ), 0.0 ) );    // multiple_pole
                }
            }

            return pole;
        }

        std::vector< std::complex< double > > FilterParam::pole_odd( const std::vector< double >& coef ) const
        {
            using std::abs;    // c++でdouble型でも使用可能
            using std::complex;
            using std::sqrt;

            const double acc = 1.0e-10;    // 1.0×10^-10≒0とし、これを基準に場合分けを行う

            std::vector< complex< double > > pole;
            pole.reserve( 2 * m_order + 1 );

            pole.emplace_back( -coef.at( n_order + 1 ) );

            for ( unsigned int m = n_order + 2; m < opt_order(); m += 2 )
            {
                double disc = coef.at( m ) * coef.at( m ) - 4 * coef.at( m + 1 );

                if ( disc < -acc )    // 複素解
                {
                    pole.emplace_back( complex< double >( -0.5 * coef.at( m ), 0.5 * sqrt( abs( disc ) ) ) );    // plus_pole
                    pole.emplace_back( conj( pole.back() ) );                                                    // minus_pole
                }
                else if ( disc > acc )    // 実数解
                {
                    double disc_sqrt = sqrt( disc );
                    pole.emplace_back( complex< double >( 0.5 * ( -coef.at( m ) + disc_sqrt ), 0.0 ) );    // plus_pole
                    pole.emplace_back( complex< double >( 0.5 * ( -coef.at( m ) - disc_sqrt ), 0.0 ) );    // minus_pole
                }
                else    // 重解
                {
                    pole.emplace_back( complex< double >( -0.5 * coef.at( m ), 0.0 ) );    // multiple_pole
                }
            }

            return pole;
        }

        std::vector< std::complex< double > > FilterParam::zero_even( const std::vector< double >& coef ) const
        {
            using std::abs;    // c++でdouble型でも使用可能
            using std::complex;
            using std::sqrt;

            const double acc = 1.0e-10;    // 1.0×10^-10≒0とし、これを基準に場合分けを行う

            std::vector< complex< double > > zero;
            zero.reserve( 2 * n_order );

            for ( unsigned int n = 1; n < n_order; n += 2 )
            {
                double disc = coef.at( n ) * coef.at( n ) - 4 * coef.at( n + 1 );

                if ( disc < -acc )    // 複素解
                {
                    zero.emplace_back( complex< double >( -0.5 * coef.at( n ), 0.5 * sqrt( abs( disc ) ) ) );    // plus_zero
                    zero.emplace_back( conj( zero.back() ) );                                                    // minus_zero
                }
                else if ( disc > acc )    // 実数解
                {
                    double disc_sqrt = sqrt( disc );
                    zero.emplace_back( complex< double >( 0.5 * ( -coef.at( n ) + disc_sqrt ), 0.0 ) );    // plus_zero
                    zero.emplace_back( complex< double >( 0.5 * ( -coef.at( n ) - disc_sqrt ), 0.0 ) );    // minus_zero
                }
                else    // 重解
                {
                    zero.emplace_back( complex< double >( -0.5 * coef.at( n ), 0.0 ) );    // multiple_zero
                }
            }

            return zero;
        }

        std::vector< std::complex< double > > FilterParam::zero_odd( const std::vector< double >& coef ) const
        {
            using std::abs;    // c++でdouble型でも使用可能
            using std::complex;
            using std::sqrt;

            const double acc = 1.0e-10;    // 1.0×10^-10≒0とし、これを基準に場合分けを行う

            std::vector< complex< double > > zero;
            zero.reserve( 2 * n_order + 1 );

            zero.emplace_back( -coef.at( 1 ) );

            for ( unsigned int n = 2; n < n_order; n += 2 )
            {
                double disc = coef.at( n ) * coef.at( n ) - 4 * coef.at( n + 1 );

                if ( disc < -acc )    // 複素解
                {
                    zero.emplace_back( complex< double >( -0.5 * coef.at( n ), 0.5 * sqrt( abs( disc ) ) ) );    // plus_zero
                    zero.emplace_back( conj( zero.back() ) );                                                    // minus_zero
                }
                else if ( disc > acc )    // 実数解
                {
                    double disc_sqrt = sqrt( disc );
                    zero.emplace_back( complex< double >( 0.5 * ( -coef.at( n ) + disc_sqrt ), 0.0 ) );    // plus_zero
                    zero.emplace_back( complex< double >( 0.5 * ( -coef.at( n ) - disc_sqrt ), 0.0 ) );    // minus_zero
                }
                else    // 重解
                {
                    zero.emplace_back( complex< double >( -0.5 * coef.at( n ), 0.0 ) );    // multiple_zero
                }
            }

            return zero;
        }

    }    // namespace iir
}    // namespace filter

std::string BandParam::sprint()
{
    std::string str;
    switch ( band_type )
    {
        case BandType::Pass:
            {
                str = format( "PassBand(%6.3f, %6.3f)", left_side, right_side );
                break;
            }
        case BandType::Stop:
            {
                str = format( "StopBand(%6.3f, %6.3f)", left_side, right_side );
                break;
            }
        case BandType::Transition:
            {
                str = format(
                    "TransitionBand(%6.3f, %6.3f)", left_side, right_side );
                break;
            }
        default:
            {
                fprintf(
                    stderr, "Error: [%s l.%d]Undefined band.\n", __FILE__,
                    __LINE__ );
                exit( EXIT_FAILURE );
            }
    }
    return str;
}
