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
    BcfReader();
    ~BcfReader();

    // Add bedfile for tabix index filtering
    void set_regions (const std::string& bedfile, bool overlap = false);

    // Add file reader
    void add_reader (const std::string& fname);

    /* CONST STATS */

    // Get errnum
    bcf_sr_error errnum() const;
    std::string errstr() const;

    // Get number of file readers added
    size_t num_files() const;

    // Get BCF header
    bcf_hdr_t* get_header(uint32_t idx = 0) const;

    /* READING */

    // Go to next record
    bool next();

    // Get data for n-th file, defaulting to 0th for
    // convenience in the common case where only one
    // file is being read
    bcf1_t* get_line(uint32_t idx = 0) const;

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
