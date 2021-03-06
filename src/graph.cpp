// Copyright 2017 Riku Walve

#include <vector>
#include <string>

#include <sdsl/bit_vectors.hpp>
#include <sdsl/lcp.hpp>

#include "index.h"
#include "interval.h"
#include "graph.h"

// TODO: Use a more efficient construction based on FM-index
sdsl::rrr_vector<127> graph_t::build_first(const std::string &filename, const size_t k) {
  sdsl::lcp_wt<> lcp;
  sdsl::construct(lcp, filename.c_str(), 1);

  sdsl::bit_vector first = sdsl::bit_vector(lcp.size(), false);
  for (size_t i = 1; i < lcp.size(); i++) {
    if (lcp[i] < k) {
      first[i-1] = true;
    }
  }

  sdsl::util::clear(lcp);

  sdsl::rrr_vector<127> rrr(first);
  sdsl::util::clear(first);

  return rrr;
}

std::string graph_t::label(const interval_t &node) const {
  interval_t interval = node;
  uint8_t c = '\0';
  for (size_t i = 0; i < m_k; i++) {
    #ifdef DEBUG
      std::cerr << "[D::" << __func__ << "]: " << i << ", "
        "(" << interval.left << ", " << interval.right << "), " <<
        static_cast<char>(c) << std::endl;
    #endif

    interval = m_index.inverse_lf(interval, &c);
    m_buffer[i] = static_cast<char>(c);

    if (c == MARKER) {
      return "";
    }
  }

  return std::string(m_buffer, m_k);
}

std::vector<interval_t> graph_t::distinct_kmers(const size_t solid) const {
  std::vector<interval_t> kmers;
  for (size_t i = 1; i <= m_first_rs.rank(m_first.size()); i++) {
    const size_t start = m_first_ss.select(i);
    const size_t end = m_first_ss.select(i + 1) - 1;

    if ((end - start) + 1 >= solid) {
      kmers.push_back(interval_t(start, end));
    }
  }
  return kmers;
}

interval_t graph_t::follow_edge(const interval_t &node, const uint8_t c) const {
  // First find the inteval e corresponding to c1 .. ck+1
  const interval_t e = m_index.extend(node, c);

  if (m_first[e.left] && (e.right == m_index.size()+1 || m_first[e.right+1]))
    return interval_t(e.left, e.right);

  // Take interval inside it corresponding to c2 .. ck+1
  const size_t start = m_first[e.left] ? e.left : m_first_ss.select(m_first_rs.rank(e.left));
  const size_t end = (e.right == m_index.size()) ? e.right : m_first_ss.select(m_first_rs.rank(e.right) + 1) - 1;

  return interval_t(start, end);
}

// std::vector<interval_t> graph_t::incoming(const interval_t &node, const size_t solid) const {
//   const std::vector<uint8_t> symbols = m_index.interval_symbols(node.left, node.right);
//
//   std::vector<interval_t> nodes;
//   for (size_t i = 0; i < symbols.size(); i++) {
//     if (symbols[i] != 0 && symbols[i] != MARKER) {
//       #ifdef DEBUG
//         std::cerr << "[D::" << __func__ << "]: " <<
//           "(" << node.left << ", " << node.right << "), " << symbols[i] << std::endl;
//       #endif
//
//       const interval_t neighbor = follow_edge(node, symbols[i]);
//       if (frequency(neighbor) >= solid) {
//         nodes.push_back(neighbor);
//       }
//     }
//   }
//
//   return nodes;
// }

// TODO: These two implementations are super dumb and ineffiecient, but accurate
std::vector<interval_t> graph_t::incoming(const interval_t &node, const size_t solid) const {
  std::vector<interval_t> edges;
  for (size_t i = node.left; i <= node.right; i++) {
    const size_t lf = m_index.lf(i);
    if (lf == 0) continue;

    const size_t rank = m_first_rs.rank(lf+1);

    const size_t left = m_first_ss.select(rank);
    const size_t right = m_first_ss.select(rank + 1) - 1;

    const interval_t n = interval_t(left, right);
    if (frequency(n) >= solid) {
      bool in = false;
      for (size_t j = 0; j < edges.size(); j++) {
        if (edges[j] == n) in = true;
      }
      if (!in) edges.push_back(n);
    }
  }

  return edges;
}

std::vector<interval_t> graph_t::outgoing(const interval_t &node, const size_t solid) const {
  std::vector<interval_t> edges;
  for (size_t i = node.left; i <= node.right; i++) {
    const size_t ilf = m_index.inverse_lf(i);
    if (ilf == 0) continue;

    const size_t rank = m_first_rs.rank(ilf+1);

    const size_t left = m_first_ss.select(rank);
    const size_t right = m_first_ss.select(rank + 1) - 1;

    const interval_t n = interval_t(left, right);
    if (frequency(n) >= solid) {
      bool in = false;
      for (size_t j = 0; j < edges.size(); j++) {
        if (edges[j] == n) in = true;
      }
      if (!in) edges.push_back(n);
    }

    // i += right - ilf + 1;
  }

  return edges;
}
