#pragma once

#include <cstdint>
#include <string>

#include "weavebound/render_graph/builder.hpp"
#include "weavebound/render_graph/compiled.hpp"
#include "weavebound/render_graph/ir.hpp"

namespace weavebound::rg {

/**
 * 輕量相容層：匿名 pass 堆疊；新程式請優先使用 RenderGraphBuilder + compile()。
 */
class RenderGraph {
 public:
  RenderGraph() = default;

  void reset() {
    impl_ = RenderGraphBuilder();
    anon_ = 0;
  }

  std::uint32_t pass_count() const { return static_cast<std::uint32_t>(impl_.passes().size()); }

  void add_pass(PassKind kind) {
    impl_.add_pass(std::string("anon_pass_") + std::to_string(anon_++), kind);
  }

  /** 底層 builder（進階用法）。 */
  RenderGraphBuilder& builder() { return impl_; }
  const RenderGraphBuilder& builder() const { return impl_; }

 private:
  RenderGraphBuilder impl_;
  std::uint32_t anon_{0};
};

}  // namespace weavebound::rg
