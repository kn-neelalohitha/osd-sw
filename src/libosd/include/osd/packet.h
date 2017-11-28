/* Copyright 2017 The Open SoC Debug Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OSD_PACKET_H
#define OSD_PACKET_H

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup libosd-packet Packet
 * @ingroup libosd
 *
 * @{
 */

/**
 * A packet in the Open SoC Debug system
 */
// We must use zero-length data members (a GCC extension) instead of standard C
// flexible arrays (uint16_t payload[]) as flexible array members are not
// allowed in unions by the C standard.
struct osd_packet {
    uint16_t data_size_words;  //!< size of data/data_raw in uint16_t words
    union {
        struct {
            uint16_t dest;        //!< packet destination address
            uint16_t src;         //!< packet source address
            uint16_t flags;       //!< packet flags
            uint16_t payload[0];  //!< (size_data_words - 3) words of payload
        } data;

        uint16_t data_raw[0];  //!< size_data words of data
    };
};

/**
 * Packet types
 */
enum osd_packet_type {
    OSD_PACKET_TYPE_REG = 0,    //< Register access
    OSD_PACKET_TYPE_PLAIN = 1,  //< Plain (unspecified content)
    OSD_PACKET_TYPE_EVENT = 2,  //< Debug Event
    OSD_PACKET_TYPE_RES = 3     //< Reserved (will be discarded)
};

/**
 * Values of the TYPE_SUB field in if TYPE == OSD_PACKET_TYPE_REG
 */
enum osd_packet_type_reg_subtype {
    REQ_READ_REG_16 = 0b0000,            //< 16 bit register read request
    REQ_READ_REG_32 = 0b0001,            //< 32 bit register read request
    REQ_READ_REG_64 = 0b0010,            //< 64 bit register read request
    REQ_READ_REG_128 = 0b0011,           //< 128 bit register read request
    RESP_READ_REG_SUCCESS_16 = 0b1000,   //< 16 bit register read response
    RESP_READ_REG_SUCCESS_32 = 0b1001,   //< 32 bit register read response
    RESP_READ_REG_SUCCESS_64 = 0b1010,   //< 64 bit register read response
    RESP_READ_REG_SUCCESS_128 = 0b1011,  //< 128 bit register read response
    RESP_READ_REG_ERROR = 0b1100,        //< register read failure
    REQ_WRITE_REG_16 = 0b0100,           //< 16 bit register write request
    REQ_WRITE_REG_32 = 0b0101,           //< 32 bit register write request
    REQ_WRITE_REG_64 = 0b0110,           //< 64 bit register write request
    REQ_WRITE_REG_128 = 0b0111,          //< 128 bit register write request
    RESP_WRITE_REG_SUCCESS = 0b1110,     //< the preceding write request was
                                         //  successful
    RESP_WRITE_REG_ERROR = 0b1111        //< the preceding write request failed
};

// debug packet structure
#define DP_HEADER_TYPE_SHIFT 14
#define DP_HEADER_TYPE_MASK 0b11

#define DP_HEADER_TYPE_SUB_SHIFT 10
#define DP_HEADER_TYPE_SUB_MASK 0b1111

#define DP_HEADER_SRC_SHIFT 0
#define DP_HEADER_SRC_MASK ((1 << 16) - 1)

#define DP_HEADER_DEST_SHIFT 0
#define DP_HEADER_DEST_MASK ((1 << 16) - 1)

/**
 * Allocate memory for a packet with given data size and zero all data fields
 *
 * The osd_packet.size field is set to the allocated size.
 *
 * @param[out] packet the packet to be allocated
 * @param[in]  size_data_words number of uint16_t words in the packet, including
 *             the header words.
 * @return the allocated packet, or NULL if allocation fails
 */
osd_result osd_packet_new(struct osd_packet **packet, size_t size_data_words);

/**
 * Create a new packet from a zframe
 *
 * @see osd_packet_new()
 */
osd_result osd_packet_new_from_zframe(struct osd_packet **packet,
                                      const zframe_t *frame);

/**
 * Free the memory associated with the packet and NULL the object
 */
void osd_packet_free(struct osd_packet **packet);

/**
 * Extract the DEST field out of a packet
 */
unsigned int osd_packet_get_dest(const struct osd_packet *packet);

/**
 * Extract the SRC field out of a packet
 */
unsigned int osd_packet_get_src(const struct osd_packet *packet);

/**
 * Extract the TYPE field out of a packet
 */
unsigned int osd_packet_get_type(const struct osd_packet *packet);

/**
 * Extract the TYPE_SUB field out of a packet
 */
unsigned int osd_packet_get_type_sub(const struct osd_packet *packet);

/**
 * Populate the header of a osd_packet
 *
 * @param packet
 * @param dest     packet destination
 * @param src      packet source
 * @param type     packet type
 * @param type_sub packet subtype
 *
 * @return OSD_OK on success, any other value indicates an error
 */
osd_result osd_packet_set_header(struct osd_packet *packet,
                                 const unsigned int dest,
                                 const unsigned int src,
                                 const enum osd_packet_type type,
                                 const unsigned int type_sub);

/**
 * Size in bytes of a packet
 */
size_t osd_packet_sizeof(const struct osd_packet *packet);

/**
 * Get the data size including all headers for a given payload size
 */
const uint16_t osd_packet_get_data_size_words_from_payload(
    const unsigned int size_payload);

/**
 * Write a debug message to the log with the contents of a packet
 */
void osd_packet_log(const struct osd_packet *packet,
                    struct osd_log_ctx *log_ctx);

/**
 * Dump a packet in human-readable (debugging) form to a file stream
 *
 * @param packet packet to dump
 * @param fd stream to dump packet to. You can use stdout and stderr here.
 *
 * @see osd_packet_to_string()
 */
void osd_packet_dump(const struct osd_packet *packet, FILE *fd);

/**
 * Dump the packet to a string (for human consumption)
 *
 * The string representation of a packet is for debugging purposes only and may
 * change at any time, do not rely on it for automated parsing.
 *
 * @see osd_packet_dump()
 */
void osd_packet_to_string(const struct osd_packet *packet, char **str);

/**@}*/ /* end of doxygen group libosd-packet */

#ifdef __cplusplus
}
#endif

#endif  // OSD_PACKET_H