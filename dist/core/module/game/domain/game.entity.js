"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.EGameStatus = void 0;
var EGameStatus;
(function (EGameStatus) {
    EGameStatus[EGameStatus["AWAITING"] = 1] = "AWAITING";
    EGameStatus[EGameStatus["IN_PROGRESS"] = 2] = "IN_PROGRESS";
    EGameStatus[EGameStatus["FINISHED"] = 3] = "FINISHED";
})(EGameStatus || (exports.EGameStatus = EGameStatus = {}));
