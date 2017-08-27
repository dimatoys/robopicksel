#include "utils/image.h"

void TObjectValidator::ValidateBackgroundBySize(unsigned area,
                                                int border,
                                                TValidationResult& result) {
	result.LikelyBackground = area > ImageWidth * ImageHeight * MinBackgroundAreaPct ? 0.9 : 0.1;
}

AreaType TObjectValidator::ClassifyArea(TArea* area) {
	double objWidth = (area->MaxX - area->MinX) / ImageWidth;
	double objHeight = (area->MaxY - area->MinY) / ImageHeight;
	if ((objWidth >= 0.2) && (objHeight >= 0.2)) {
		if ((objWidth <= 0.7) && (objHeight <= 0.7)) {
			return LIKELY_OBJECT;
		}

		if ((objWidth >= 0.95) && (objHeight >=0.95)) {
			return LIKELY_BACKGROUD;
		}

		return LIKELY_INCOMPLETED_BACKGROUND;

	} else {
		//return LIKELY_FALSE_DETECTION;
		return LIKELY_OBJECT;
	}
}

int TObjectValidator::ValidateArea(TArea* area) {
	double areaPct = ((area->MaxX - area->MinX) / ImageWidth) * ((area->MaxY - area->MinY) / ImageHeight);

	if (areaPct >= MinObjectAreaPct) {
		if(areaPct <= MaxObjectAreaPct) {
			return 0;
		} else {
			return 1;
		}
	} else {
		return -1;
	}
}
