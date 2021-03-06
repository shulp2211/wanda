// Copyright 2017 Riku Walve

#include <cassert>
#include <cstring>

#include <iostream>
#include <string>
#include <vector>

// #include <sdsl/bit_vectors.hpp>

#define BUFFER_SIZE 1024*1024*32
#define SEPARATOR "$"

void next_line(FILE *in, char *buffer) {
  size_t buffer_len = strlen(buffer);
  while (buffer[buffer_len-1] != '\n') {
    fgets(buffer, BUFFER_SIZE, in);
    buffer_len = strlen(buffer);
  }
}

void rev_comp(char *buffer, char *tmp) {
  const size_t len = strlen(buffer);
  for (size_t i = 0; i < len; i++) {
    switch (buffer[len - 1 - i]) {
      case 'A': tmp[i] = 'T'; break;
      case 'T': tmp[i] = 'A'; break;
      case 'G': tmp[i] = 'C'; break;
      case 'C': tmp[i] = 'G'; break;
    }
  }
  memcpy(buffer, tmp, len);
}

// Concatenates FASTA files into a stream-like format
size_t concatenate(const char *filename, const bool type, FILE *out, const bool reverse) {
  // TODO: Support gzipped files with zlib deflate
  FILE *in = fopen(filename, "r");
  if (in == NULL) {
    std::cerr << "Error opening file: " << filename;
    exit(EXIT_FAILURE);
  }

  char *buffer = new char[BUFFER_SIZE];
  char *tmp = new char[BUFFER_SIZE];

  size_t length = 0;
  while (fgets(buffer, BUFFER_SIZE, in)) {
    size_t buffer_len = strlen(buffer);
    if (type) {
      // NOTE: Assume single-line FASTQ files
      assert(buffer_len == 1 || buffer[0] == '@');
      if (buffer_len == 1) continue;

      // Skip sequnce id
      next_line(in, buffer);

      if (ftell(out) > 0) {
        fputs(SEPARATOR, out);
        length++;
      }

      // Write sequence
      fgets(buffer, BUFFER_SIZE, in);
      buffer_len = strlen(buffer);
      if (buffer[buffer_len-1] == '\n') {
        buffer[buffer_len-1] = '\0';
      }

      if (reverse) {
        rev_comp(buffer, tmp);
      }

      fputs(buffer, out);

      // Skip rest of the sequnce data
      //next_line(in, buffer);
      fgets(buffer, BUFFER_SIZE, in);
      assert(buffer[0] == '+');
      //next_line(in, buffer);
      fgets(buffer, BUFFER_SIZE, in);
    } else {
      // Skip FASTA sequence name
      if (buffer[0] == '>') {
        next_line(in, buffer);

        if (ftell(out) > 0) {
          fputs(SEPARATOR, out);
          length++;
        }
      } else {
        // Join multi-line sequences
        if (buffer[buffer_len-1] == '\n') {
          buffer[buffer_len-1] = '\0';
        }

        if (reverse) {
          rev_comp(buffer, tmp);
        }

        fputs(buffer, out);
        length += buffer_len;
      }
    }
  }

  fputs(SEPARATOR, out);
  length++;

  fclose(in);
  delete[] buffer;

  return length;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " <out> <file1[,<01>]> [file2] ..." << std::endl;
    return 1;
  }

  // std::vector<size_t> colors = { 0 };
  FILE *out = fopen(argv[1], "w");
  for (int i = 2; i < argc; i++) {
    size_t name_length = strlen(argv[i]);

    bool reverse = false;
    if (argv[i][name_length-2] == ',') {
      reverse = (argv[i][name_length-1] == '1');
      name_length -= 2;
      argv[i][name_length] = '\0';
    }

    bool fastq = strcmp(argv[i] + (name_length - 2), "fq") == 0;

    const size_t length = concatenate(argv[i], fastq, out, reverse);
    // colors.push_back(colors.back() + length);
  }

  fclose(out);

  // sdsl::bit_vector colors_bv = sdsl::bit_vector(len, false);
  // sdsl::sd_vector<> colors_sd = sdsl::sd_vector<>(colors_bv);
  // sdsl::store_to_file(colors_sd, filename + ".stream.colors");

  return 0;
}
