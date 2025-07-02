#ifndef DICOMHELPERS_H
#define DICOMHELPERS_H

extern "C" {
#include <dicom/dicom.h>
}

#include <cstdint>
#include <list>
#include <string>
#include <vector>

// #### direct IO access ####

// ## low level ##
char *readVR(DcmIO *io);
uint32_t readTag(DcmIO *io);
uint32_t readLength(DcmIO *io, uint32_t tag);
// ## hight level ##
DcmElement *readDataElement(DcmIO *io, uint32_t tag, bool explicitVR);
DcmSequence *readSequence(DcmIO *io, uint32_t tag, bool explicitVR);

// #### dataset access ####
std::string getString(const DcmDataSet *dataset, uint32_t tag);
int64_t getNumber(const DcmDataSet *dataset, uint32_t tag);

std::list<std::vector<char>> getFrames(DcmDataSet *dataset,
                                       const DcmDataSet *meta, DcmIO *io,
                                       DcmFilehandle *filehandle,
                                       std::string &msg);

// callback function that can be used for dcm_dataset_foreach
// useful for debugging purposes
// the callback data is a FILE* of an opened file
bool print_element(const DcmElement *element, void *data);

#endif // DICOMHELPERS_H
