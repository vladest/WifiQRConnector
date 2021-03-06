#pragma once
/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
* Copyright 2021 Axel Waggershauser
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "ConcentricFinder.h"

#include <vector>

namespace ZXing {

class DetectorResult;
class BitMatrix;

namespace QRCode {

struct FinderPatternSet
{
	ConcentricPattern bl, tl, tr;
};

using FinderPatternSets = std::vector<FinderPatternSet>;

FinderPatternSets FindFinderPatternSets(const BitMatrix& image, bool tryHarder);
DetectorResult SampleAtFinderPatternSet(const BitMatrix& image, const FinderPatternSet& fp);

/**
 * @brief Detects a QR Code in an image.
 */
DetectorResult Detect(const BitMatrix& image, bool tryHarder, bool isPure);

} // QRCode
} // ZXing
