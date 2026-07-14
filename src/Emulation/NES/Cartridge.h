#pragma once
#include <memory>
#include <string>

#include "Mappers/Mapper.h"

namespace Nes {

void LoadCardDb(const std::string& path);
std::shared_ptr<Mapper> LoadCart(const std::string& path);

}
