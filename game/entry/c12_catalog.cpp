#include "c12_catalog.h"

#include "c12_runtime.h"
#include "executable_identity.h"
#include "single_title_catalog.h"

namespace c12 {
namespace {

constexpr psx::host::TitleIdentity kTitleIdentity{
    .displayName = "C-12: Final Resistance",
    .serial = identity::kSerial,
    .slug = "c12",
    .fileSize = identity::kExecutableBytes,
    .sha256 = identity::kExecutableSha256,
    .entry = identity::kEntry,
    .globalPointer = identity::kGlobalPointer,
    .textAddress = identity::kTextAddress,
    .textSize = identity::kTextBytes,
    .stackAddress = identity::kStackAddress,
    .stackOffset = identity::kStackOffset,
    .discExecutablePath = identity::kDiscExecutablePath,
};

} // namespace

std::unique_ptr<psx::host::TitleCatalog> createCatalog() {
  return std::make_unique<psx::host::SingleTitleCatalog>(kTitleIdentity, "scratch/bin", std::make_unique<C12Runtime>());
}

} // namespace c12
