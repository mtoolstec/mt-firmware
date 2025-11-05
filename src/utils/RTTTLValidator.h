#pragma once

#include <cstring>

/**
 * RTTTL (Ring Tone Text Transfer Language) format validator
 * Validates and parses RTTTL strings according to the format:
 * <name>:d=<default duration>,o=<default octave>,b=<bpm>:<data>
 */
class RTTTLValidator
{
  public:
    /**
     * Validates if the given string is a valid RTTTL format
     * @param rtttl The RTTTL string to validate
     * @return true if valid RTTTL format, false otherwise
     */
    static bool isValidRTTTL(const char *rtttl);

    /**
     * Extracts the RTTTL data part (after the colon) from a complete RTTTL string
     * @param rtttl The complete RTTTL string
     * @param buffer Buffer to store the extracted RTTTL data
     * @param bufferSize Size of the buffer
     * @return true if successful, false otherwise
     */
    static bool extractRTTTLData(const char *rtttl, char *buffer, size_t bufferSize);

    /**
     * Parses a message for embedded RTTTL content and play count
     * Supports formats: "buz:RTTTL..." and "Message:name:d=X,o=X,b=X:notes..."
     * @param message The full message text
     * @param rtttlOutput Buffer to store the extracted RTTTL
     * @param rtttlOutputSize Size of the RTTTL output buffer
     * @param playCount Pointer to store the play count (default 1)
     * @return true if parsing was successful, false otherwise
     */
    static bool parseBuzMessage(const char *message, char *rtttlOutput, size_t rtttlOutputSize, int *playCount);

    /**
     * Removes RTTTL content from message text for display purposes
     * Converts "Message:name:d=X,o=X,b=X:notes..." to "Message(name)"
     * @param message The original message
     * @param cleanOutput Buffer to store the cleaned message
     * @param cleanOutputSize Size of the clean output buffer
     * @return true if cleaning was successful, false otherwise
     */
    static bool cleanMessageForDisplay(const char *message, char *cleanOutput, size_t cleanOutputSize);

  private:
    /**
     * Validates the header part of RTTTL (name:d=X,o=X,b=X)
     * @param header The header string to validate
     * @return true if valid header, false otherwise
     */
    static bool isValidHeader(const char *header);

    /**
     * Validates the data part of RTTTL (note sequence)
     * @param data The data string to validate
     * @return true if valid data, false otherwise
     */
    static bool isValidData(const char *data);

    /**
     * Checks if a character is a valid note (a-g, A-G, p, P)
     * @param c The character to check
     * @return true if valid note, false otherwise
     */
    static bool isValidNote(char c);

    /**
     * Checks if a character is a valid octave (4-7)
     * @param c The character to check
     * @return true if valid octave, false otherwise
     */
    static bool isValidOctave(char c);
};