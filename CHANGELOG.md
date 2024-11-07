# Changelog
All notable changes to this project will be documented in this file. See [conventional commits](https://www.conventionalcommits.org/) for commit guidelines.

- - -
## 0.3.11 - 2024-11-07
#### Bug Fixes
- capability safeties (#156) - (a25ad82) - Austin Kelway
#### Features
- Allow readonly of stream components even when toggled (#155) - (cb60b50) - Austin Kelway

- - -

## 0.3.10 - 2024-10-29
#### Bug Fixes
- fauly logic on index of get components (#154) - (5b751ce) - Austin Kelway

- - -

## 0.3.9 - 2024-10-26
#### Bug Fixes
- stream toggle codegen misses (#153) - (46e7c76) - Ezekiel Warren
- bad codegen with multiple stream_toggle systems - (2fb568d) - Ezekiel

- - -

## 0.3.8 - 2024-10-17
#### Features
- toggle stream + new indexed fields api (#151) - (1d79758) - Austin Kelway
#### Miscellaneous Chores
- **(deps)** update ecsact repositories (#150) - (17c4531) - renovate[bot]
- **(deps)** update dependency ecsact_runtime to v0.6.8 (#148) - (ac49d43) - renovate[bot]
- **(deps)** update dependency rules_ecsact to v0.5.6 (#147) - (c2b7394) - renovate[bot]
- improve compile commands situation in main - (dc5fdbb) - Ezekiel Warren
- sync with ecsact_common (#149) - (fe02ca1) - seaubot

- - -

## 0.3.7 - 2024-08-10
#### Miscellaneous Chores
- refactor to use new codegen api (#146) - (fb7b0d9) - Ezekiel Warren

- - -

## 0.3.6 - 2024-08-09
#### Bug Fixes
- Let tag components call get_component, add missing export (#145) - (f30e725) - Austin Kelway
#### Miscellaneous Chores
- **(deps)** update dependency ecsact_cli to v0.3.15 (#144) - (c88cc16) - renovate[bot]

- - -

## 0.3.5 - 2024-08-07
#### Bug Fixes
- remove duplicate recipe source files (#143) - (6307675) - Ezekiel Warren
#### Miscellaneous Chores
- **(deps)** update dependency ecsact_cli to v0.3.14 (#142) - (6056028) - renovate[bot]
- update readme logo - (1625505) - Ezekiel Warren

- - -

## 0.3.4 - 2024-07-03
#### Miscellaneous Chores
- **(deps)** update ecsact repositories (#134) - (2f3f565) - renovate[bot]
- add build recipe for rt entt to be used (#140) - (c0cbe43) - Austin Kelway

- - -

## 0.3.3 - 2024-07-02
#### Bug Fixes
- generate add events not calling, parent trivial systems children systems not running (#136) - (4d596c3) - Austin Kelway
- dynamic view gets (#123) - (f45ca6c) - Austin Kelway
- proper internal comparor (#121) - (39b0505) - Ezekiel Warren
- generates missing internal sorting (#115) - (e4440d3) - Ezekiel Warren
- update events happening too often (#119) - (fa98277) - Ezekiel Warren
- generates invalid callback (#113) - (5ed2115) - Ezekiel Warren
- entity created callback called too often (#111) - (f5b8693) - Ezekiel Warren
#### Features
- respect parallel system parameter (#127) - (9845dd5) - Ezekiel Warren
- new assoc api placeholder (#139) - (4cdd656) - Ezekiel Warren
- parallel entities (#129) - (10c39a7) - Austin Kelway
- parallel execution (#124) - (dee2b1e) - Austin Kelway
- adding onchange for systems (#114) - (eff4582) - Austin Kelway
- add reactive systems with init, update and remove as initial features (#110) - (5223bcf) - Austin Kelway
- add ecsact bazel build recipe (#104) - (f0484dc) - Ezekiel Warren
- enable all features with ecsact build (#105) - (b1ce5cc) - Ezekiel Warren
#### Miscellaneous Chores
- **(deps)** update rules_ecsact to 0.5.2 (#107) - (effe481) - Ezekiel Warren
- **(deps)** update dependency googletest to v1.14.0.bcr.1 (#103) - (1730aca) - renovate[bot]
- **(deps)** update dependency bazel_skylib to v1.6.1 (#98) - (c781778) - renovate[bot]
- **(deps)** update dependency bazel to v7.1.2 (#99) - (280a007) - renovate[bot]
- sync with ecsact_common (#137) - (7bda631) - seaubot
- sync with ecsact_common (#135) - (c46a61c) - seaubot
- sync with ecsact_common (#112) - (f946a24) - seaubot
- exception for renovate bot (#133) - (01155ad) - Ezekiel Warren
- system providers (#126) - (d324920) - Austin Kelway
- Cleanup print_sys_exec (#125) - (6a5d6fc) - Austin Kelway
- add some convenient system impl macros - (0f71d39) - Ezekiel Warren
- sync with ecsact_common (#109) - (09e842e) - seaubot
- faster ci with conditionals (#108) - (3dba899) - Ezekiel Warren
- faster ci + no lock file (#106) - (28bf8e7) - Ezekiel Warren

- - -

## 0.3.2 - 2024-05-13
#### Features
- add build recipe and tests (#100) - (2fb7f19) - Austin Kelway

- - -

## 0.3.1 - 2024-04-22
#### Bug Fixes
- check for created callback nullptr (#97) - (9601a75) - Ezekiel Warren
#### Miscellaneous Chores
- update lock file in cog.toml - (5471f71) - Ezekiel Warren
- update bazel lock - (a1d61d9) - Ezekiel Warren

- - -

## 0.3.0 - 2024-04-20
#### Features
- sorted lazy systems (#95) - (1356b93) - Ezekiel Warren
- lazy systems (#94) - (b94cfcb) - Ezekiel Warren
- ecsact codegen plugin for optimization (#56) - (177250a) - Ezekiel Warren
#### Miscellaneous Chores
- **(deps)** update dependency bazel to v7.1.1 (#88) - (a91f6f0) - renovate[bot]
- **(deps)** update hedron_compile_commands digest to eca42c6 (#83) - (b1713bd) - renovate[bot]
- **(deps)** update bazel c++ tooling repositories (#80) - (2e0a8e1) - renovate[bot]
- sync with ecsact_common (#77) - (e6befa7) - seaubot
- fix typos (#92) - (8ec3473) - Ezekiel Warren
- use ecsact_cli for toolchain (#90) - (c4ef016) - Ezekiel Warren
- cleanup line endings (#91) - (1c8a0a9) - Ezekiel Warren
- bzlmod updates - (6342148) - Ezekiel Warren

- - -

## 0.2.2 - 2023-09-22
#### Bug Fixes
- remove toolchain config ecsact (#79) - (55afa5e) - Ezekiel Warren

- - -

## 0.2.1 - 2023-09-21
#### Features
- bzlmodify (#78) - (dee1e89) - Ezekiel Warren
#### Miscellaneous Chores
- **(deps)** update com_grail_bazel_toolchain digest to 2733561 (#75) - (b614711) - renovate[bot]
- **(deps)** update com_grail_bazel_toolchain digest to 3e6d4d9 (#74) - (b15e8d7) - renovate[bot]
- **(deps)** update com_grail_bazel_toolchain digest to edc058a (#73) - (de146ad) - renovate[bot]
- **(deps)** update com_grail_bazel_toolchain digest to e4fad4e (#72) - (7ddf8cb) - renovate[bot]
- **(deps)** update com_grail_bazel_toolchain digest to c217c03 (#71) - (0955a83) - renovate[bot]
- **(deps)** update com_grail_bazel_toolchain digest to 9d8cc8a (#69) - (dcb5966) - renovate[bot]
- **(deps)** update bazel c++ tooling repositories (#68) - (2bf2c9a) - renovate[bot]
- **(deps)** update bazel c++ tooling repositories (#66) - (b337746) - renovate[bot]

- - -

## 0.2.0 - 2023-06-24
#### Bug Fixes
- tag components can now be added (#62) - (dd15872) - Ezekiel Warren
#### Features
- **(performance)** context fn indirect invocation instead of exhaust checks (#46) - (59390ea) - Ezekiel Warren
#### Miscellaneous Chores
- **(deps)** update com_grail_bazel_toolchain digest to ceeedcc (#61) - (3ed0739) - renovate[bot]
- **(deps)** update bazel c++ tooling repositories (#60) - (0947067) - renovate[bot]
- **(deps)** ecsact_cli toolchain + update deps (#59) - (0defb9e) - Ezekiel Warren
- **(deps)** update EnTT - (96d175e) - Ezekiel Warren
- **(deps)** update bazel c++ tooling repositories (#49) - (7d15bff) - renovate[bot]
- refactor entity entt <-> ecsact entity IDs (#57) - (46e66f0) - Ezekiel Warren
- little bit of cleanup (#55) - (b91a459) - Ezekiel Warren

- - -

Changelog generated by [cocogitto](https://github.com/cocogitto/cocogitto).
