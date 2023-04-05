#include <iostream>
#include <fstream>
#include <cstdio>
#include <memory>
#include "cli_utils.h"
#include "catch.hpp"
#include "spdlog/sinks/null_sink.h"

using namespace std;
using namespace GLnexus;
using namespace GLnexus::cli;

static auto console = spdlog::create<spdlog::sinks::null_sink_st>("test_cli_utils_null");

TEST_CASE("cli_utils") {
    unsigned N;
    vector<pair<string,size_t>> contigs;
    contigs.push_back(make_pair("16",12345));
    contigs.push_back(make_pair("17",23456));

    const char* da_yaml1 = 1 + R"(
- range: {ref: '16', beg: 100, end: 100}
  dna: A
  is_ref: true
  all_filtered: false
  top_AQ: [99]
  zygosity_by_GQ: [[100,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0]]
- range: {ref: '16', beg: 113, end: 120}
  dna: G
  is_ref: false
  all_filtered: false
  top_AQ: [99]
  zygosity_by_GQ: [[0,0],[10,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,5]]
)";

    const char* da_yaml2 = 1 + R"(
- range: {ref: '17', beg: 100, end: 100}
  dna: A
  is_ref: true
  all_filtered: false
  top_AQ: [99]
  zygosity_by_GQ: [[100,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0]]
- range: {ref: '17', beg: 200, end: 310}
  dna: G
  is_ref: false
  all_filtered: false
  top_AQ: [99]
  zygosity_by_GQ: [[0,0],[10,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,5]]
)";

    const char* da_yaml3 = 1 + R"(
- range: {ref: '16', beg: 107, end: 109}
  dna: A
  is_ref: true
  all_filtered: false
  top_AQ: [99]
  zygosity_by_GQ: [[100,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0]]
- range: {ref: '17', beg: 200, end: 310}
  dna: G
  is_ref: false
  all_filtered: false
  top_AQ: [99]
  zygosity_by_GQ: [[0,0],[20,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,0],[0,5]]
)";

    SECTION("parse_range") {
        GLnexus::range query(-1,-1,-1);
        string range_txt = "17:100-2000";
        REQUIRE(utils::parse_range(contigs, range_txt, query));

        range_txt = "20:10000-30000";
        REQUIRE(!utils::parse_range(contigs, range_txt, query));

        range_txt = "17:0-10000000";
        REQUIRE(!utils::parse_range(contigs, range_txt, query));

        string cmdline("xxx yyy");
        vector<range> ranges;
        REQUIRE(!utils::parse_ranges(contigs, cmdline, ranges));

        cmdline = string("16:10-37,17:901-3220");
        REQUIRE(utils::parse_ranges(contigs, cmdline, ranges));
    }

    SECTION("yaml_discovered_alleles, an empty list") {
        // an empty list
        std::stringstream ss;
        {
            GLnexus::discovered_alleles dsals;
            Status s = utils::yaml_stream_of_discovered_alleles(1, contigs, dsals, ss);
            REQUIRE(s.ok());
        }
        discovered_alleles dal_empty;
        vector<pair<string,size_t>> contigs2;
        Status s = utils::discovered_alleles_of_yaml_stream(ss, N, contigs2, dal_empty);
        REQUIRE(s.ok());
        REQUIRE(N == 1);
        REQUIRE(contigs == contigs2);
    }

    SECTION("yaml_discovered_alleles, an empty contigs") {
        Status s;

        discovered_alleles dal1;
        vector<pair<string,size_t>> contigs_empty;
        std::stringstream ss;
        s = utils::yaml_stream_of_discovered_alleles(0, contigs_empty, dal1, ss);
        REQUIRE(s.ok());

        discovered_alleles dal_empty;
        vector<pair<string,size_t>> contigs2;
        s = utils::discovered_alleles_of_yaml_stream(ss, N, contigs2, dal_empty);
        REQUIRE(s.bad());
    }

    SECTION("yaml_discovered_alleles") {
        discovered_alleles dsals;
        {
            YAML::Node n = YAML::Load(da_yaml1);
            discovered_alleles dal1;
            Status s = discovered_alleles_of_yaml(n, contigs, dal1);
            REQUIRE(s.ok());
            merge_discovered_alleles(dal1, dsals);

            n = YAML::Load(da_yaml2);
            discovered_alleles dal2;
            s = discovered_alleles_of_yaml(n, contigs, dal2);
            REQUIRE(s.ok());
            merge_discovered_alleles(dal2, dsals);
        }

        std::stringstream ss;
        Status s = utils::yaml_stream_of_discovered_alleles(1, contigs, dsals, ss);
        REQUIRE(s.ok());

        std::vector<std::pair<std::string,size_t> > contigs2;
        discovered_alleles dsals2;
        s = utils::discovered_alleles_of_yaml_stream(ss, N, contigs2, dsals2);
        REQUIRE(N == 1);
        REQUIRE(contigs == contigs2);
        REQUIRE(dsals.size() == dsals2.size());
    }

    const char* bad_yaml_1 = 1 + R"(
contigs: xxx
alleles: yyy
)";

    const char* bad_yaml_2 = 1 + R"(
contigs:
  - name: A1
    size: 1000
  - name: A2
    size: 300000
alleles: yyy
)";


    SECTION("bad yaml inputs for discovered_alleles_of_yaml_stream") {
        discovered_alleles dsals;
        {
            stringstream ss("aaa");
            Status s = utils::discovered_alleles_of_yaml_stream(ss, N, contigs, dsals);
            REQUIRE(s.bad());
        }

        {
            stringstream ss(bad_yaml_1);
            Status s = utils::discovered_alleles_of_yaml_stream(ss, N, contigs, dsals);
            REQUIRE(s.bad());
        }

        {
            stringstream ss(bad_yaml_2);
            Status s = utils::discovered_alleles_of_yaml_stream(ss, N, contigs, dsals);
            REQUIRE(s.bad());
        }

        {
            std::stringstream ss;
            ss.write("---\n", 4);
            ss.write("xxx\n", 4);
            ss.write("yyy\n", 4);
            ss.write("zzz\n", 4);
            ss.write("----\n", 5);
            ss.write("zzz\n", 4);
            ss.write("...\n", 5);

            discovered_alleles dal_empty;
            vector<pair<string,size_t>> contigs2;
            Status s = utils::discovered_alleles_of_yaml_stream(ss, N, contigs2, dal_empty);
            REQUIRE(s.bad());
        }

        // Good contigs, but bad discovered alleles
        {
            std::stringstream ss;
            {
                GLnexus::discovered_alleles dsals;
                Status s = utils::yaml_stream_of_discovered_alleles(1, contigs, dsals, ss);
                REQUIRE(s.ok());
            }

            // Extra characters after EOF, that should not be there
            ss << "ssss" << endl;

            discovered_alleles dal_empty;
            vector<pair<string,size_t>> contigs2;
            Status s = utils::discovered_alleles_of_yaml_stream(ss, N, contigs2, dal_empty);
            REQUIRE(s.bad());
        }
    }


    const char* snp = 1 + R"(
range: {ref: '17', beg: 100, end: 100}
alleles:
- dna: A
- dna: G
  frequency: 0.51
quality: 317
unification: []
)";


    const char* del = 1 + R"(
range: {ref: '17', beg: 1000, end: 1001}
alleles:
- dna: AG
- dna: AC
  frequency: 0.50
- dna: C
  frequency: 0.1
quality: 317
unification:
  - range: {ref: '17', beg: 1000, end: 1001}
    dna: C
    to: 2
)";

    SECTION("yaml_of_unified_sites") {
        // generate a vector of sites
        vector<unified_site> sites;
        {
            YAML::Node n = YAML::Load(snp);
            unified_site us(range(-1,-1,-1));
            Status s = unified_site::of_yaml(n, contigs, us);
            REQUIRE(s.ok());
            sites.push_back(us);

            n = YAML::Load(del);
            s = unified_site::of_yaml(n, contigs, us);
            REQUIRE(s.ok());
            sites.push_back(us);
        }

        // convert to yaml
        stringstream ss;
        {
            Status s = utils::yaml_stream_of_unified_sites(sites, contigs, ss);
            REQUIRE(s.ok());
        }

        // convert back and compare
        {
            vector<unified_site> sites2;
            Status s = utils::unified_sites_of_yaml_stream(ss, contigs, sites2);
            REQUIRE(s.ok());
            REQUIRE(sites.size() == sites2.size());
            for (int i=0; i < sites.size(); i++) {
                REQUIRE(sites[i] == sites2[i]);
            }
        }
    }

    SECTION("LoadYAMLFile") {
        string tmp_file_name = "/tmp/xxx.yml";
        std::remove(tmp_file_name.c_str());

        {
            YAML::Node node;
            string emptyname = "";
            Status s = utils::LoadYAMLFile(emptyname, node);
            REQUIRE(s.bad());
        }

        // Create a trivial YAML file
        {
            YAML::Emitter yaml;

            yaml << YAML::BeginSeq;
            yaml << "x";
            yaml << "y";
            yaml << "z";
            yaml << YAML::EndSeq;

            ofstream fos;
            fos.open(tmp_file_name);
            fos << yaml.c_str();
            fos.close();
        }

        // verify the file
        {
            YAML::Node node;
            Status s = utils::LoadYAMLFile(tmp_file_name, node);
            REQUIRE(s.ok());
        }

        // Create an invalid YAML file
        std::remove(tmp_file_name.c_str());
        {
            ofstream fos;
            fos.open(tmp_file_name);
            fos << "hello: false:" << endl;
            fos << "world: oyster" << endl;
            fos << "movies: Dune Airplane Princess Bride" << endl;
            fos.close();
        }

        // verify the file does not load
        {
            YAML::Node node;
            Status s = utils::LoadYAMLFile(tmp_file_name, node);
            REQUIRE(s.bad());
        }

        // Create an invalid YAML file
        std::remove(tmp_file_name.c_str());
        {
            ofstream fos;
            fos.open(tmp_file_name);
            fos << "" << endl;
            fos.close();
        }

        // verify the file does not load
        {
            YAML::Node node;
            Status s = utils::LoadYAMLFile(tmp_file_name, node);
            REQUIRE(s.bad());
        }

    }
}

TEST_CASE("find_target_range") {
    vector<pair<string,size_t>> contigs;
    contigs.push_back(make_pair("1",100000));
    contigs.push_back(make_pair("2",130001));
    contigs.push_back(make_pair("3",24006));

    SECTION("5 ranges") {
        std::set<range> ranges;
        ranges.insert(range(1,15,20));
        ranges.insert(range(1,108,151));
        ranges.insert(range(2,31,50));
        ranges.insert(range(2,42,48));
        ranges.insert(range(3,1000,1507));

        Status s;
        range ans(-1,-1,-1);

        // beginning
        range pos(1,17,18);
        s = find_target_range(ranges, pos, ans);
        REQUIRE(s.ok());
        REQUIRE(ans == range(1,15,20));

        // last
        pos = range(3,1200,1218);
        s = find_target_range(ranges, pos, ans);
        REQUIRE(s.ok());
        REQUIRE(ans == range(3,1000,1507));

        pos = range(3,1000,1001);
        s = find_target_range(ranges, pos, ans);
        REQUIRE(s.ok());
        REQUIRE(ans == range(3,1000,1507));

        // middle
        pos = range(2,31,35);
        s = find_target_range(ranges, pos, ans);
        REQUIRE(s.ok());
        REQUIRE(ans == range(2,31,50));

        // missing range
        pos = range(2,301,302);
        s = find_target_range(ranges, pos, ans);
        REQUIRE(s.bad());

        // missing range
        pos = range(3,2000,2003);
        s = find_target_range(ranges, pos, ans);
        REQUIRE(s.bad());

        // missing range
        pos = range(1,2,4);
        s = find_target_range(ranges, pos, ans);
        REQUIRE(s.bad());

        // dangling not within (from the left)
        pos = range(1,105,110);
        s = find_target_range(ranges, pos, ans);
        REQUIRE(s.ok());
        REQUIRE(ans == range(1,108,151));

        // dangling to the right
        pos = range(1,150,155);
        s = find_target_range(ranges, pos, ans);
        REQUIRE(s.ok());
        REQUIRE(ans == range(1,108,151));
    }

    SECTION("1 range") {
        std::set<range> ranges;
        ranges.insert(range(1,15,20));

        Status s;
        range ans(-1,-1,-1);

        // beginning
        range pos(1,17,18);
        s = find_target_range(ranges, pos, ans);
        REQUIRE(s.ok());
        REQUIRE(ans == range(1,15,20));

        // missing range
        pos = range(3,1200,1218);
        s = find_target_range(ranges, pos, ans);
        REQUIRE(s.bad());

        // missing range
        pos = range(1,2,5);
        s = find_target_range(ranges, pos, ans);
        REQUIRE(s.bad());

        // dangling
        pos = range(1,15,30);
        s = find_target_range(ranges, pos, ans);
        REQUIRE(s.ok());
    }

    SECTION("no ranges") {
        Status s;
        range ans(-1,-1,-1);
        std::set<range> ranges;

        range pos(1,17,18);
        s = find_target_range(ranges, pos, ans);
        REQUIRE(s.bad());
    }

    SECTION("larger example") {
        vector<pair<string,size_t>> contigs;
        contigs.push_back(make_pair("1",260000000));
        contigs.push_back(make_pair("10",140000000));
        contigs.push_back(make_pair("11",100000000));

        set<range> ranges;
        ranges.insert(range(1,30366,30503));
        ranges.insert(range(1,35720,35737));
        ranges.insert(range(1,69090,70009));
        ranges.insert(range(1,249152026,249152059));
        ranges.insert(range(1,249208061,249208079));
        ranges.insert(range(1,249210800,249214146));
        ranges.insert(range(2,92996,94055));
        ranges.insert(range(2,95121,95179));
        ranges.insert(range(2,255828,255989));
        ranges.insert(range(2,135480471,135481678));
        ranges.insert(range(2,135491589,135491842));
        ranges.insert(range(3,168957,169053));
        ranges.insert(range(3,180208,180405));
        ranges.insert(range(3,5685264,5685404));
        ranges.insert(range(3,5700990,5701408));
        ranges.insert(range(3,5717462,5717886));

        range pos(1, 249211350, 249211350);
        range ans(-1,-1,-1);
        Status s = find_target_range(ranges,pos,ans);
        REQUIRE(s.ok());
        REQUIRE(ans == range(1,249210800,249214146));
    }
}

static void make_files_in_dir(const string &basedir) {
    auto names = {"a", "b", "c"};
    for (auto cursor : names) {
        string path = basedir + "/" + cursor;
        int retval = system(("touch " + path).c_str());
        REQUIRE(retval == 0);
        REQUIRE(utils::check_file_exists(path));
    }
}

TEST_CASE("file_ops") {
    string basedir = "/tmp/cli_utils";
    int retval = system(("rm -rf " + basedir).c_str());
    REQUIRE(retval == 0);
    retval = system(("mkdir -p " + basedir).c_str());
    REQUIRE(retval == 0);
    REQUIRE(utils::check_dir_exists(basedir));

    string file_path = basedir + "/foo.txt";
    retval = system(("touch " + file_path).c_str());
    REQUIRE(retval == 0);
    REQUIRE(utils::check_file_exists(file_path));

    make_files_in_dir(basedir);

    // create some subdirectories
    auto subdirs = {"X", "Y", "Z"};
    for (auto cursor : subdirs) {
        string path = basedir + "/" + cursor;
        retval = system(("mkdir " + path).c_str());
        REQUIRE(retval == 0);

        make_files_in_dir(path);
    }

    retval = system(("rm -rf " + basedir).c_str());
    REQUIRE(retval == 0);
    REQUIRE(!utils::check_dir_exists(basedir));
}

TEST_CASE("parse_bed_file") {
    Status s;

    vector<pair<string,size_t>> contigs;
    contigs.push_back(make_pair("1",10000000));
    contigs.push_back(make_pair("2",31000000));

    string basedir = "test/data/cli";
    string bedfilename = basedir + "/vcr_test.bed";
    vector<range> ranges;
    s = utils::parse_bed_file(console, bedfilename, contigs, ranges);
    REQUIRE(s.ok());
    REQUIRE(ranges.size() == 10);
}

TEST_CASE("iter_compare") {
    Status s;

    // setup database directory
    string dbdir = "/tmp/iter_compare";
    REQUIRE(system(("rm -rf " + dbdir).c_str()) == 0);
    REQUIRE(system(("mkdir -p " + dbdir).c_str()) == 0);
    string dbpath = dbdir + "/DB";

    string basedir = "test/data/cli";
    string exemplar_gvcf = basedir + "/" + "F1.gvcf.gz";
    vector<pair<string,size_t>> contigs;
    s = cli::utils::db_init(console, dbpath, exemplar_gvcf, contigs);
    REQUIRE(s.ok());
    REQUIRE(contigs.size() >= 1);

    vector<string> gvcfs;
    for (auto fname : {"F1.gvcf.gz", "F2.gvcf.gz"}) {
         gvcfs.push_back(basedir + "/" + fname);
    }
    std::string emptybed = "";
    s = cli::utils::db_bulk_load(console,  0, 8, gvcfs, dbpath, emptybed, contigs);
    REQUIRE(s.ok());
    REQUIRE(contigs.size() >= 1);

    int n_iter = 50;
    s = cli::utils::compare_db_itertion_algorithms(console, dbpath, n_iter);
    console->info("Passed {} iterator comparison tests", n_iter);
}
