#include <vcf.h>
#include <synced_bcf_reader.h>

#include <string>
#include <vector>

#include "data.h"
#include "types.h"

namespace GLnexus {

    // C++ wrapper around synced_bcf_reader tools

class BcfReader {
public:
    // Basic constructor and destroyer
    BcfReader() 
     : sr(nullptr),
       nfiles(0),
       reading(false)
    {
        sr = bcf_sr_init();
        if (!sr) {
            throw std::runtime_error("Could not initialize bcf_srs_t handle");
        }
    }
    ~BcfReader() {
        if (sr) {
            bcf_sr_destroy(sr);
        }
    }
    // Add region
    void set_regions (const std::string& bedfile, bool overlap = false) {
        if (nfiles > 0 || reading) {
            throw std::runtime_error("set_regions MUST be called before the first call to add_reader");
        }
        if (overlap) {
            //bcf_sr_set_opt(sr, BCF_SR_REGIONS_OVERLAP, 1); // TODO: requires a newer version of htslib
        }
        int ret = bcf_sr_set_regions(sr, bedfile.c_str(), 1);
        if (ret < 0) { // 0 on success, -1 on failure
            throw std::runtime_error("Could not set reader regions");
        }
    }
    // Add file reader
    void add_reader (const std::string& fname)  {
        if (reading) {
            throw std::runtime_error("BCF iteration has begun; cannot add new bcfs");
        }
        int ret = bcf_sr_add_reader(sr, fname.c_str());
        if (ret == 0) { // 1 on success, 0 on failure
            throw std::runtime_error("Could not add reader for file");
        }
        ++nfiles;
    }

    /* CONST STATS */

    // Get errnum
    bcf_sr_error errnum() const {
        return sr->errnum;
    }
    std::string errstr() const {
        return bcf_sr_strerror(sr->errnum);
    }
    // Get number of file readers added
    size_t num_files() const {
        return nfiles;
    }
    // Get BCF header
    bcf_hdr_t* get_header(uint32_t idx = 0) const {
        if (idx > nfiles) {
            throw std::invalid_argument("Cannot retrieve header; no file with this index belongs to the reader");
        }
        return bcf_sr_get_header(sr, idx);
    }

    /* READING */

    // Go to next record
    bool next() {
        reading = true; // can no longer add files
        return bcf_sr_next_line(sr);
    }
    // Get data for n-th file, defaulting to 0th for
    // convenience in the common case where only one
    // file is being read
    bcf1_t* get_line(uint32_t idx = 0) const {
        if (!reading) {
            // can't start getting lines until calling next(), i.e. in a while loop
            throw std::invalid_argument("Cannot retrieve record; iteration has not yet begun");
        }
        if (idx > nfiles) {
            throw std::invalid_argument("Cannot retrieve record; no file with this index belongs to the reader");
        }
        return bcf_sr_get_line(sr,idx);
    }
private:
    bcf_srs_t* sr;
    size_t nfiles;
    bool reading;
};

// test whether a gVCF file is compatible for deposition into the database.
bool gvcf_compatible(const MetadataCache& metadata, const bcf_hdr_t* hdr);

// hard-coded xAtlas ingestion exceptions
// filter VRFromDeletion: accessory information
// format VR+RR >= 65536: unreliable QC values
bool xAtlas_ingestion_exceptions(const bcf_hdr_t* hdr, bcf1_t* bcf);

// Sanity-check an individual bcf1_t record before ingestion.
Status validate_bcf(const std::vector<std::pair<std::string, size_t>>& contigs,
                    const std::string& filename, const bcf_hdr_t* hdr,
                    bcf1_t* bcf, int prev_rid, int prev_pos,
                    bool& skip_ingestion);

// Verify that a VCF file is well formed.
// AND, fill in the [samples_out]
Status vcf_validate_basic_facts(MetadataCache& metadata,
                                const std::string& dataset,
                                const std::string& filename,
                                const BcfReader& vcf,
                                std::set<std::string>& samples_out);

}  // namespace GLnexus
