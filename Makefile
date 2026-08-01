# SPDX-License-Identifier: Apache-2.0

.PHONY: diagrams diagrams-clean diagrams-check

diagrams:
	bash ./scripts/generate_diagrams.sh

diagrams-clean:
	bash ./scripts/clean_diagrams.sh

diagrams-check:
	bash ./scripts/check_diagrams_up_to_date.sh
