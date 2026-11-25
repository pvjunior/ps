#pragma once

#include "LoaderTypes.h"

ObjectParseResult parseObjectFile(const std::string& path);
ObjectParseResult parseObjectText(const std::string& text,
                                 const std::string& sourceName = "<memory>");
