"use strict";
module.exports = validate10;
module.exports.default = validate10;
const schema11 = {
  $schema: "http://json-schema.org/draft-06/schema#",
  $ref: "#/definitions/Welcome",
  definitions: {
    Welcome: {
      type: "object",
      additionalProperties: false,
      properties: {
        base: { $ref: "#/definitions/Alpha" },
        alpha: { $ref: "#/definitions/Alpha" },
        delta: { $ref: "#/definitions/Alpha" },
        omicron_ba1: { $ref: "#/definitions/Alpha" },
        omicron_ba2: { $ref: "#/definitions/Alpha" },
        omicron_ba4_5: { $ref: "#/definitions/Alpha" },
      },
      required: [
        "alpha",
        "base",
        "delta",
        "omicron_ba1",
        "omicron_ba2",
        "omicron_ba4_5",
      ],
      title: "Welcome",
    },
    Alpha: {
      type: "object",
      additionalProperties: false,
      properties: {
        spread: { $ref: "#/definitions/Spread" },
        immunity: { $ref: "#/definitions/Immunity" },
        progression_tree: {
          anyOf: [
            {
              type: "object",
              additionalProperties: {
                type: "object",
                additionalProperties: { $ref: "#/definitions/ProgressionTree" },
              },
            },
            { type: "null" },
          ],
        },
        progression_factors: { $ref: "#/definitions/ProgressionFactors" },
      },
      required: [
        "immunity",
        "progression_factors",
        "progression_tree",
        "spread",
      ],
      title: "Alpha",
    },
    Immunity: {
      type: "object",
      additionalProperties: false,
      properties: {
        recovery_immunity: { $ref: "#/definitions/RecoveryImmunity" },
        immunehalflife: { type: "integer" },
      },
      required: ["immunehalflife", "recovery_immunity"],
      title: "Immunity",
    },
    RecoveryImmunity: {
      type: "object",
      additionalProperties: false,
      properties: {
        base: { type: "number" },
        alpha: { type: "number" },
        delta: { type: "number" },
        omicron_ba1: { type: "number" },
        omicron_ba2: { type: "number" },
        omicron_ba4_5: { type: "number" },
      },
      required: [
        "alpha",
        "base",
        "delta",
        "omicron_ba1",
        "omicron_ba2",
        "omicron_ba4_5",
      ],
      title: "RecoveryImmunity",
    },
    ProgressionFactors: {
      type: "object",
      additionalProperties: false,
      properties: {
        riskadjust: { type: "array", items: { type: "number" } },
        vaxhalflifeadjust: { $ref: "#/definitions/Vaxhalflifeadjust" },
      },
      required: ["riskadjust", "vaxhalflifeadjust"],
      title: "ProgressionFactors",
    },
    Vaxhalflifeadjust: {
      type: "object",
      additionalProperties: false,
      properties: {
        JnJ: { type: "number" },
        Pfizer: { type: "number" },
        Moderna: { type: "number" },
      },
      required: ["JnJ", "Moderna", "Pfizer"],
      title: "Vaxhalflifeadjust",
    },
    ProgressionTree: {
      type: "object",
      additionalProperties: false,
      properties: {
        nil: { type: "array", items: { type: "number" } },
        mild: { type: "array", items: { type: "number" } },
        sick: { type: "array", items: { type: "number" } },
        severe: { type: "array", items: { type: "number" } },
      },
      required: ["mild", "nil", "severe", "sick"],
      title: "ProgressionTree",
    },
    Spread: {
      type: "object",
      additionalProperties: false,
      properties: {
        sendrisk: { type: "array", items: { type: "number" } },
        recvrisk: { type: "array", items: { type: "number" } },
        basemultiplier: { type: "number" },
      },
      required: ["basemultiplier", "recvrisk", "sendrisk"],
      title: "Spread",
    },
  },
};
const schema12 = {
  type: "object",
  additionalProperties: false,
  properties: {
    base: { $ref: "#/definitions/Alpha" },
    alpha: { $ref: "#/definitions/Alpha" },
    delta: { $ref: "#/definitions/Alpha" },
    omicron_ba1: { $ref: "#/definitions/Alpha" },
    omicron_ba2: { $ref: "#/definitions/Alpha" },
    omicron_ba4_5: { $ref: "#/definitions/Alpha" },
  },
  required: [
    "alpha",
    "base",
    "delta",
    "omicron_ba1",
    "omicron_ba2",
    "omicron_ba4_5",
  ],
  title: "Welcome",
};
const schema13 = {
  type: "object",
  additionalProperties: false,
  properties: {
    spread: { $ref: "#/definitions/Spread" },
    immunity: { $ref: "#/definitions/Immunity" },
    progression_tree: {
      anyOf: [
        {
          type: "object",
          additionalProperties: {
            type: "object",
            additionalProperties: { $ref: "#/definitions/ProgressionTree" },
          },
        },
        { type: "null" },
      ],
    },
    progression_factors: { $ref: "#/definitions/ProgressionFactors" },
  },
  required: ["immunity", "progression_factors", "progression_tree", "spread"],
  title: "Alpha",
};
const schema14 = {
  type: "object",
  additionalProperties: false,
  properties: {
    sendrisk: { type: "array", items: { type: "number" } },
    recvrisk: { type: "array", items: { type: "number" } },
    basemultiplier: { type: "number" },
  },
  required: ["basemultiplier", "recvrisk", "sendrisk"],
  title: "Spread",
};
const schema17 = {
  type: "object",
  additionalProperties: false,
  properties: {
    nil: { type: "array", items: { type: "number" } },
    mild: { type: "array", items: { type: "number" } },
    sick: { type: "array", items: { type: "number" } },
    severe: { type: "array", items: { type: "number" } },
  },
  required: ["mild", "nil", "severe", "sick"],
  title: "ProgressionTree",
};
const schema15 = {
  type: "object",
  additionalProperties: false,
  properties: {
    recovery_immunity: { $ref: "#/definitions/RecoveryImmunity" },
    immunehalflife: { type: "integer" },
  },
  required: ["immunehalflife", "recovery_immunity"],
  title: "Immunity",
};
const schema16 = {
  type: "object",
  additionalProperties: false,
  properties: {
    base: { type: "number" },
    alpha: { type: "number" },
    delta: { type: "number" },
    omicron_ba1: { type: "number" },
    omicron_ba2: { type: "number" },
    omicron_ba4_5: { type: "number" },
  },
  required: [
    "alpha",
    "base",
    "delta",
    "omicron_ba1",
    "omicron_ba2",
    "omicron_ba4_5",
  ],
  title: "RecoveryImmunity",
};
function validate13(
  data,
  { instancePath = "", parentData, parentDataProperty, rootData = data } = {},
) {
  let vErrors = null;
  let errors = 0;
  if (errors === 0) {
    if (data && typeof data == "object" && !Array.isArray(data)) {
      let missing0;
      if (
        (data.immunehalflife === undefined && (missing0 = "immunehalflife")) ||
        (data.recovery_immunity === undefined &&
          (missing0 = "recovery_immunity"))
      ) {
        validate13.errors = [
          {
            instancePath,
            schemaPath: "#/required",
            keyword: "required",
            params: { missingProperty: missing0 },
            message: "must have required property '" + missing0 + "'",
          },
        ];
        return false;
      } else {
        const _errs1 = errors;
        for (const key0 in data) {
          if (!(key0 === "recovery_immunity" || key0 === "immunehalflife")) {
            validate13.errors = [
              {
                instancePath,
                schemaPath: "#/additionalProperties",
                keyword: "additionalProperties",
                params: { additionalProperty: key0 },
                message: "must NOT have additional properties",
              },
            ];
            return false;
            break;
          }
        }
        if (_errs1 === errors) {
          if (data.recovery_immunity !== undefined) {
            let data0 = data.recovery_immunity;
            const _errs2 = errors;
            const _errs3 = errors;
            if (errors === _errs3) {
              if (data0 && typeof data0 == "object" && !Array.isArray(data0)) {
                let missing1;
                if (
                  (data0.alpha === undefined && (missing1 = "alpha")) ||
                  (data0.base === undefined && (missing1 = "base")) ||
                  (data0.delta === undefined && (missing1 = "delta")) ||
                  (data0.omicron_ba1 === undefined &&
                    (missing1 = "omicron_ba1")) ||
                  (data0.omicron_ba2 === undefined &&
                    (missing1 = "omicron_ba2")) ||
                  (data0.omicron_ba4_5 === undefined &&
                    (missing1 = "omicron_ba4_5"))
                ) {
                  validate13.errors = [
                    {
                      instancePath: instancePath + "/recovery_immunity",
                      schemaPath: "#/definitions/RecoveryImmunity/required",
                      keyword: "required",
                      params: { missingProperty: missing1 },
                      message: "must have required property '" + missing1 + "'",
                    },
                  ];
                  return false;
                } else {
                  const _errs5 = errors;
                  for (const key1 in data0) {
                    if (
                      !(
                        key1 === "base" ||
                        key1 === "alpha" ||
                        key1 === "delta" ||
                        key1 === "omicron_ba1" ||
                        key1 === "omicron_ba2" ||
                        key1 === "omicron_ba4_5"
                      )
                    ) {
                      validate13.errors = [
                        {
                          instancePath: instancePath + "/recovery_immunity",
                          schemaPath:
                            "#/definitions/RecoveryImmunity/additionalProperties",
                          keyword: "additionalProperties",
                          params: { additionalProperty: key1 },
                          message: "must NOT have additional properties",
                        },
                      ];
                      return false;
                      break;
                    }
                  }
                  if (_errs5 === errors) {
                    if (data0.base !== undefined) {
                      let data1 = data0.base;
                      const _errs6 = errors;
                      if (!(typeof data1 == "number" && isFinite(data1))) {
                        validate13.errors = [
                          {
                            instancePath:
                              instancePath + "/recovery_immunity/base",
                            schemaPath:
                              "#/definitions/RecoveryImmunity/properties/base/type",
                            keyword: "type",
                            params: { type: "number" },
                            message: "must be number",
                          },
                        ];
                        return false;
                      }
                      var valid2 = _errs6 === errors;
                    } else {
                      var valid2 = true;
                    }
                    if (valid2) {
                      if (data0.alpha !== undefined) {
                        let data2 = data0.alpha;
                        const _errs8 = errors;
                        if (!(typeof data2 == "number" && isFinite(data2))) {
                          validate13.errors = [
                            {
                              instancePath:
                                instancePath + "/recovery_immunity/alpha",
                              schemaPath:
                                "#/definitions/RecoveryImmunity/properties/alpha/type",
                              keyword: "type",
                              params: { type: "number" },
                              message: "must be number",
                            },
                          ];
                          return false;
                        }
                        var valid2 = _errs8 === errors;
                      } else {
                        var valid2 = true;
                      }
                      if (valid2) {
                        if (data0.delta !== undefined) {
                          let data3 = data0.delta;
                          const _errs10 = errors;
                          if (!(typeof data3 == "number" && isFinite(data3))) {
                            validate13.errors = [
                              {
                                instancePath:
                                  instancePath + "/recovery_immunity/delta",
                                schemaPath:
                                  "#/definitions/RecoveryImmunity/properties/delta/type",
                                keyword: "type",
                                params: { type: "number" },
                                message: "must be number",
                              },
                            ];
                            return false;
                          }
                          var valid2 = _errs10 === errors;
                        } else {
                          var valid2 = true;
                        }
                        if (valid2) {
                          if (data0.omicron_ba1 !== undefined) {
                            let data4 = data0.omicron_ba1;
                            const _errs12 = errors;
                            if (
                              !(typeof data4 == "number" && isFinite(data4))
                            ) {
                              validate13.errors = [
                                {
                                  instancePath:
                                    instancePath +
                                    "/recovery_immunity/omicron_ba1",
                                  schemaPath:
                                    "#/definitions/RecoveryImmunity/properties/omicron_ba1/type",
                                  keyword: "type",
                                  params: { type: "number" },
                                  message: "must be number",
                                },
                              ];
                              return false;
                            }
                            var valid2 = _errs12 === errors;
                          } else {
                            var valid2 = true;
                          }
                          if (valid2) {
                            if (data0.omicron_ba2 !== undefined) {
                              let data5 = data0.omicron_ba2;
                              const _errs14 = errors;
                              if (
                                !(typeof data5 == "number" && isFinite(data5))
                              ) {
                                validate13.errors = [
                                  {
                                    instancePath:
                                      instancePath +
                                      "/recovery_immunity/omicron_ba2",
                                    schemaPath:
                                      "#/definitions/RecoveryImmunity/properties/omicron_ba2/type",
                                    keyword: "type",
                                    params: { type: "number" },
                                    message: "must be number",
                                  },
                                ];
                                return false;
                              }
                              var valid2 = _errs14 === errors;
                            } else {
                              var valid2 = true;
                            }
                            if (valid2) {
                              if (data0.omicron_ba4_5 !== undefined) {
                                let data6 = data0.omicron_ba4_5;
                                const _errs16 = errors;
                                if (
                                  !(typeof data6 == "number" && isFinite(data6))
                                ) {
                                  validate13.errors = [
                                    {
                                      instancePath:
                                        instancePath +
                                        "/recovery_immunity/omicron_ba4_5",
                                      schemaPath:
                                        "#/definitions/RecoveryImmunity/properties/omicron_ba4_5/type",
                                      keyword: "type",
                                      params: { type: "number" },
                                      message: "must be number",
                                    },
                                  ];
                                  return false;
                                }
                                var valid2 = _errs16 === errors;
                              } else {
                                var valid2 = true;
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              } else {
                validate13.errors = [
                  {
                    instancePath: instancePath + "/recovery_immunity",
                    schemaPath: "#/definitions/RecoveryImmunity/type",
                    keyword: "type",
                    params: { type: "object" },
                    message: "must be object",
                  },
                ];
                return false;
              }
            }
            var valid0 = _errs2 === errors;
          } else {
            var valid0 = true;
          }
          if (valid0) {
            if (data.immunehalflife !== undefined) {
              let data7 = data.immunehalflife;
              const _errs18 = errors;
              if (
                !(
                  typeof data7 == "number" &&
                  !(data7 % 1) &&
                  !isNaN(data7) &&
                  isFinite(data7)
                )
              ) {
                validate13.errors = [
                  {
                    instancePath: instancePath + "/immunehalflife",
                    schemaPath: "#/properties/immunehalflife/type",
                    keyword: "type",
                    params: { type: "integer" },
                    message: "must be integer",
                  },
                ];
                return false;
              }
              var valid0 = _errs18 === errors;
            } else {
              var valid0 = true;
            }
          }
        }
      }
    } else {
      validate13.errors = [
        {
          instancePath,
          schemaPath: "#/type",
          keyword: "type",
          params: { type: "object" },
          message: "must be object",
        },
      ];
      return false;
    }
  }
  validate13.errors = vErrors;
  return errors === 0;
}
const schema18 = {
  type: "object",
  additionalProperties: false,
  properties: {
    riskadjust: { type: "array", items: { type: "number" } },
    vaxhalflifeadjust: { $ref: "#/definitions/Vaxhalflifeadjust" },
  },
  required: ["riskadjust", "vaxhalflifeadjust"],
  title: "ProgressionFactors",
};
const schema19 = {
  type: "object",
  additionalProperties: false,
  properties: {
    JnJ: { type: "number" },
    Pfizer: { type: "number" },
    Moderna: { type: "number" },
  },
  required: ["JnJ", "Moderna", "Pfizer"],
  title: "Vaxhalflifeadjust",
};
function validate15(
  data,
  { instancePath = "", parentData, parentDataProperty, rootData = data } = {},
) {
  let vErrors = null;
  let errors = 0;
  if (errors === 0) {
    if (data && typeof data == "object" && !Array.isArray(data)) {
      let missing0;
      if (
        (data.riskadjust === undefined && (missing0 = "riskadjust")) ||
        (data.vaxhalflifeadjust === undefined &&
          (missing0 = "vaxhalflifeadjust"))
      ) {
        validate15.errors = [
          {
            instancePath,
            schemaPath: "#/required",
            keyword: "required",
            params: { missingProperty: missing0 },
            message: "must have required property '" + missing0 + "'",
          },
        ];
        return false;
      } else {
        const _errs1 = errors;
        for (const key0 in data) {
          if (!(key0 === "riskadjust" || key0 === "vaxhalflifeadjust")) {
            validate15.errors = [
              {
                instancePath,
                schemaPath: "#/additionalProperties",
                keyword: "additionalProperties",
                params: { additionalProperty: key0 },
                message: "must NOT have additional properties",
              },
            ];
            return false;
            break;
          }
        }
        if (_errs1 === errors) {
          if (data.riskadjust !== undefined) {
            let data0 = data.riskadjust;
            const _errs2 = errors;
            if (errors === _errs2) {
              if (Array.isArray(data0)) {
                var valid1 = true;
                const len0 = data0.length;
                for (let i0 = 0; i0 < len0; i0++) {
                  let data1 = data0[i0];
                  const _errs4 = errors;
                  if (!(typeof data1 == "number" && isFinite(data1))) {
                    validate15.errors = [
                      {
                        instancePath: instancePath + "/riskadjust/" + i0,
                        schemaPath: "#/properties/riskadjust/items/type",
                        keyword: "type",
                        params: { type: "number" },
                        message: "must be number",
                      },
                    ];
                    return false;
                  }
                  var valid1 = _errs4 === errors;
                  if (!valid1) {
                    break;
                  }
                }
              } else {
                validate15.errors = [
                  {
                    instancePath: instancePath + "/riskadjust",
                    schemaPath: "#/properties/riskadjust/type",
                    keyword: "type",
                    params: { type: "array" },
                    message: "must be array",
                  },
                ];
                return false;
              }
            }
            var valid0 = _errs2 === errors;
          } else {
            var valid0 = true;
          }
          if (valid0) {
            if (data.vaxhalflifeadjust !== undefined) {
              let data2 = data.vaxhalflifeadjust;
              const _errs6 = errors;
              const _errs7 = errors;
              if (errors === _errs7) {
                if (
                  data2 &&
                  typeof data2 == "object" &&
                  !Array.isArray(data2)
                ) {
                  let missing1;
                  if (
                    (data2.JnJ === undefined && (missing1 = "JnJ")) ||
                    (data2.Moderna === undefined && (missing1 = "Moderna")) ||
                    (data2.Pfizer === undefined && (missing1 = "Pfizer"))
                  ) {
                    validate15.errors = [
                      {
                        instancePath: instancePath + "/vaxhalflifeadjust",
                        schemaPath: "#/definitions/Vaxhalflifeadjust/required",
                        keyword: "required",
                        params: { missingProperty: missing1 },
                        message:
                          "must have required property '" + missing1 + "'",
                      },
                    ];
                    return false;
                  } else {
                    const _errs9 = errors;
                    for (const key1 in data2) {
                      if (
                        !(
                          key1 === "JnJ" ||
                          key1 === "Pfizer" ||
                          key1 === "Moderna"
                        )
                      ) {
                        validate15.errors = [
                          {
                            instancePath: instancePath + "/vaxhalflifeadjust",
                            schemaPath:
                              "#/definitions/Vaxhalflifeadjust/additionalProperties",
                            keyword: "additionalProperties",
                            params: { additionalProperty: key1 },
                            message: "must NOT have additional properties",
                          },
                        ];
                        return false;
                        break;
                      }
                    }
                    if (_errs9 === errors) {
                      if (data2.JnJ !== undefined) {
                        let data3 = data2.JnJ;
                        const _errs10 = errors;
                        if (!(typeof data3 == "number" && isFinite(data3))) {
                          validate15.errors = [
                            {
                              instancePath:
                                instancePath + "/vaxhalflifeadjust/JnJ",
                              schemaPath:
                                "#/definitions/Vaxhalflifeadjust/properties/JnJ/type",
                              keyword: "type",
                              params: { type: "number" },
                              message: "must be number",
                            },
                          ];
                          return false;
                        }
                        var valid3 = _errs10 === errors;
                      } else {
                        var valid3 = true;
                      }
                      if (valid3) {
                        if (data2.Pfizer !== undefined) {
                          let data4 = data2.Pfizer;
                          const _errs12 = errors;
                          if (!(typeof data4 == "number" && isFinite(data4))) {
                            validate15.errors = [
                              {
                                instancePath:
                                  instancePath + "/vaxhalflifeadjust/Pfizer",
                                schemaPath:
                                  "#/definitions/Vaxhalflifeadjust/properties/Pfizer/type",
                                keyword: "type",
                                params: { type: "number" },
                                message: "must be number",
                              },
                            ];
                            return false;
                          }
                          var valid3 = _errs12 === errors;
                        } else {
                          var valid3 = true;
                        }
                        if (valid3) {
                          if (data2.Moderna !== undefined) {
                            let data5 = data2.Moderna;
                            const _errs14 = errors;
                            if (
                              !(typeof data5 == "number" && isFinite(data5))
                            ) {
                              validate15.errors = [
                                {
                                  instancePath:
                                    instancePath + "/vaxhalflifeadjust/Moderna",
                                  schemaPath:
                                    "#/definitions/Vaxhalflifeadjust/properties/Moderna/type",
                                  keyword: "type",
                                  params: { type: "number" },
                                  message: "must be number",
                                },
                              ];
                              return false;
                            }
                            var valid3 = _errs14 === errors;
                          } else {
                            var valid3 = true;
                          }
                        }
                      }
                    }
                  }
                } else {
                  validate15.errors = [
                    {
                      instancePath: instancePath + "/vaxhalflifeadjust",
                      schemaPath: "#/definitions/Vaxhalflifeadjust/type",
                      keyword: "type",
                      params: { type: "object" },
                      message: "must be object",
                    },
                  ];
                  return false;
                }
              }
              var valid0 = _errs6 === errors;
            } else {
              var valid0 = true;
            }
          }
        }
      }
    } else {
      validate15.errors = [
        {
          instancePath,
          schemaPath: "#/type",
          keyword: "type",
          params: { type: "object" },
          message: "must be object",
        },
      ];
      return false;
    }
  }
  validate15.errors = vErrors;
  return errors === 0;
}
function validate12(
  data,
  { instancePath = "", parentData, parentDataProperty, rootData = data } = {},
) {
  let vErrors = null;
  let errors = 0;
  if (errors === 0) {
    if (data && typeof data == "object" && !Array.isArray(data)) {
      let missing0;
      if (
        (data.immunity === undefined && (missing0 = "immunity")) ||
        (data.progression_factors === undefined &&
          (missing0 = "progression_factors")) ||
        (data.progression_tree === undefined &&
          (missing0 = "progression_tree")) ||
        (data.spread === undefined && (missing0 = "spread"))
      ) {
        validate12.errors = [
          {
            instancePath,
            schemaPath: "#/required",
            keyword: "required",
            params: { missingProperty: missing0 },
            message: "must have required property '" + missing0 + "'",
          },
        ];
        return false;
      } else {
        const _errs1 = errors;
        for (const key0 in data) {
          if (
            !(
              key0 === "spread" ||
              key0 === "immunity" ||
              key0 === "progression_tree" ||
              key0 === "progression_factors"
            )
          ) {
            validate12.errors = [
              {
                instancePath,
                schemaPath: "#/additionalProperties",
                keyword: "additionalProperties",
                params: { additionalProperty: key0 },
                message: "must NOT have additional properties",
              },
            ];
            return false;
            break;
          }
        }
        if (_errs1 === errors) {
          if (data.spread !== undefined) {
            let data0 = data.spread;
            const _errs2 = errors;
            const _errs3 = errors;
            if (errors === _errs3) {
              if (data0 && typeof data0 == "object" && !Array.isArray(data0)) {
                let missing1;
                if (
                  (data0.basemultiplier === undefined &&
                    (missing1 = "basemultiplier")) ||
                  (data0.recvrisk === undefined && (missing1 = "recvrisk")) ||
                  (data0.sendrisk === undefined && (missing1 = "sendrisk"))
                ) {
                  validate12.errors = [
                    {
                      instancePath: instancePath + "/spread",
                      schemaPath: "#/definitions/Spread/required",
                      keyword: "required",
                      params: { missingProperty: missing1 },
                      message: "must have required property '" + missing1 + "'",
                    },
                  ];
                  return false;
                } else {
                  const _errs5 = errors;
                  for (const key1 in data0) {
                    if (
                      !(
                        key1 === "sendrisk" ||
                        key1 === "recvrisk" ||
                        key1 === "basemultiplier"
                      )
                    ) {
                      validate12.errors = [
                        {
                          instancePath: instancePath + "/spread",
                          schemaPath:
                            "#/definitions/Spread/additionalProperties",
                          keyword: "additionalProperties",
                          params: { additionalProperty: key1 },
                          message: "must NOT have additional properties",
                        },
                      ];
                      return false;
                      break;
                    }
                  }
                  if (_errs5 === errors) {
                    if (data0.sendrisk !== undefined) {
                      let data1 = data0.sendrisk;
                      const _errs6 = errors;
                      if (errors === _errs6) {
                        if (Array.isArray(data1)) {
                          var valid3 = true;
                          const len0 = data1.length;
                          for (let i0 = 0; i0 < len0; i0++) {
                            let data2 = data1[i0];
                            const _errs8 = errors;
                            if (
                              !(typeof data2 == "number" && isFinite(data2))
                            ) {
                              validate12.errors = [
                                {
                                  instancePath:
                                    instancePath + "/spread/sendrisk/" + i0,
                                  schemaPath:
                                    "#/definitions/Spread/properties/sendrisk/items/type",
                                  keyword: "type",
                                  params: { type: "number" },
                                  message: "must be number",
                                },
                              ];
                              return false;
                            }
                            var valid3 = _errs8 === errors;
                            if (!valid3) {
                              break;
                            }
                          }
                        } else {
                          validate12.errors = [
                            {
                              instancePath: instancePath + "/spread/sendrisk",
                              schemaPath:
                                "#/definitions/Spread/properties/sendrisk/type",
                              keyword: "type",
                              params: { type: "array" },
                              message: "must be array",
                            },
                          ];
                          return false;
                        }
                      }
                      var valid2 = _errs6 === errors;
                    } else {
                      var valid2 = true;
                    }
                    if (valid2) {
                      if (data0.recvrisk !== undefined) {
                        let data3 = data0.recvrisk;
                        const _errs10 = errors;
                        if (errors === _errs10) {
                          if (Array.isArray(data3)) {
                            var valid4 = true;
                            const len1 = data3.length;
                            for (let i1 = 0; i1 < len1; i1++) {
                              let data4 = data3[i1];
                              const _errs12 = errors;
                              if (
                                !(typeof data4 == "number" && isFinite(data4))
                              ) {
                                validate12.errors = [
                                  {
                                    instancePath:
                                      instancePath + "/spread/recvrisk/" + i1,
                                    schemaPath:
                                      "#/definitions/Spread/properties/recvrisk/items/type",
                                    keyword: "type",
                                    params: { type: "number" },
                                    message: "must be number",
                                  },
                                ];
                                return false;
                              }
                              var valid4 = _errs12 === errors;
                              if (!valid4) {
                                break;
                              }
                            }
                          } else {
                            validate12.errors = [
                              {
                                instancePath: instancePath + "/spread/recvrisk",
                                schemaPath:
                                  "#/definitions/Spread/properties/recvrisk/type",
                                keyword: "type",
                                params: { type: "array" },
                                message: "must be array",
                              },
                            ];
                            return false;
                          }
                        }
                        var valid2 = _errs10 === errors;
                      } else {
                        var valid2 = true;
                      }
                      if (valid2) {
                        if (data0.basemultiplier !== undefined) {
                          let data5 = data0.basemultiplier;
                          const _errs14 = errors;
                          if (!(typeof data5 == "number" && isFinite(data5))) {
                            validate12.errors = [
                              {
                                instancePath:
                                  instancePath + "/spread/basemultiplier",
                                schemaPath:
                                  "#/definitions/Spread/properties/basemultiplier/type",
                                keyword: "type",
                                params: { type: "number" },
                                message: "must be number",
                              },
                            ];
                            return false;
                          }
                          var valid2 = _errs14 === errors;
                        } else {
                          var valid2 = true;
                        }
                      }
                    }
                  }
                }
              } else {
                validate12.errors = [
                  {
                    instancePath: instancePath + "/spread",
                    schemaPath: "#/definitions/Spread/type",
                    keyword: "type",
                    params: { type: "object" },
                    message: "must be object",
                  },
                ];
                return false;
              }
            }
            var valid0 = _errs2 === errors;
          } else {
            var valid0 = true;
          }
          if (valid0) {
            if (data.immunity !== undefined) {
              const _errs16 = errors;
              if (
                !validate13(data.immunity, {
                  instancePath: instancePath + "/immunity",
                  parentData: data,
                  parentDataProperty: "immunity",
                  rootData,
                })
              ) {
                vErrors =
                  vErrors === null
                    ? validate13.errors
                    : vErrors.concat(validate13.errors);
                errors = vErrors.length;
              }
              var valid0 = _errs16 === errors;
            } else {
              var valid0 = true;
            }
            if (valid0) {
              if (data.progression_tree !== undefined) {
                let data7 = data.progression_tree;
                const _errs17 = errors;
                const _errs18 = errors;
                let valid5 = false;
                const _errs19 = errors;
                if (errors === _errs19) {
                  if (
                    data7 &&
                    typeof data7 == "object" &&
                    !Array.isArray(data7)
                  ) {
                    for (const key2 in data7) {
                      let data8 = data7[key2];
                      const _errs22 = errors;
                      if (errors === _errs22) {
                        if (
                          data8 &&
                          typeof data8 == "object" &&
                          !Array.isArray(data8)
                        ) {
                          for (const key3 in data8) {
                            let data9 = data8[key3];
                            const _errs25 = errors;
                            const _errs26 = errors;
                            if (errors === _errs26) {
                              if (
                                data9 &&
                                typeof data9 == "object" &&
                                !Array.isArray(data9)
                              ) {
                                let missing2;
                                if (
                                  (data9.mild === undefined &&
                                    (missing2 = "mild")) ||
                                  (data9.nil === undefined &&
                                    (missing2 = "nil")) ||
                                  (data9.severe === undefined &&
                                    (missing2 = "severe")) ||
                                  (data9.sick === undefined &&
                                    (missing2 = "sick"))
                                ) {
                                  const err0 = {
                                    instancePath:
                                      instancePath +
                                      "/progression_tree/" +
                                      key2
                                        .replace(/~/g, "~0")
                                        .replace(/\//g, "~1") +
                                      "/" +
                                      key3
                                        .replace(/~/g, "~0")
                                        .replace(/\//g, "~1"),
                                    schemaPath:
                                      "#/definitions/ProgressionTree/required",
                                    keyword: "required",
                                    params: { missingProperty: missing2 },
                                    message:
                                      "must have required property '" +
                                      missing2 +
                                      "'",
                                  };
                                  if (vErrors === null) {
                                    vErrors = [err0];
                                  } else {
                                    vErrors.push(err0);
                                  }
                                  errors++;
                                } else {
                                  const _errs28 = errors;
                                  for (const key4 in data9) {
                                    if (
                                      !(
                                        key4 === "nil" ||
                                        key4 === "mild" ||
                                        key4 === "sick" ||
                                        key4 === "severe"
                                      )
                                    ) {
                                      const err1 = {
                                        instancePath:
                                          instancePath +
                                          "/progression_tree/" +
                                          key2
                                            .replace(/~/g, "~0")
                                            .replace(/\//g, "~1") +
                                          "/" +
                                          key3
                                            .replace(/~/g, "~0")
                                            .replace(/\//g, "~1"),
                                        schemaPath:
                                          "#/definitions/ProgressionTree/additionalProperties",
                                        keyword: "additionalProperties",
                                        params: { additionalProperty: key4 },
                                        message:
                                          "must NOT have additional properties",
                                      };
                                      if (vErrors === null) {
                                        vErrors = [err1];
                                      } else {
                                        vErrors.push(err1);
                                      }
                                      errors++;
                                      break;
                                    }
                                  }
                                  if (_errs28 === errors) {
                                    if (data9.nil !== undefined) {
                                      let data10 = data9.nil;
                                      const _errs29 = errors;
                                      if (errors === _errs29) {
                                        if (Array.isArray(data10)) {
                                          var valid10 = true;
                                          const len2 = data10.length;
                                          for (let i2 = 0; i2 < len2; i2++) {
                                            let data11 = data10[i2];
                                            const _errs31 = errors;
                                            if (
                                              !(
                                                typeof data11 == "number" &&
                                                isFinite(data11)
                                              )
                                            ) {
                                              const err2 = {
                                                instancePath:
                                                  instancePath +
                                                  "/progression_tree/" +
                                                  key2
                                                    .replace(/~/g, "~0")
                                                    .replace(/\//g, "~1") +
                                                  "/" +
                                                  key3
                                                    .replace(/~/g, "~0")
                                                    .replace(/\//g, "~1") +
                                                  "/nil/" +
                                                  i2,
                                                schemaPath:
                                                  "#/definitions/ProgressionTree/properties/nil/items/type",
                                                keyword: "type",
                                                params: { type: "number" },
                                                message: "must be number",
                                              };
                                              if (vErrors === null) {
                                                vErrors = [err2];
                                              } else {
                                                vErrors.push(err2);
                                              }
                                              errors++;
                                            }
                                            var valid10 = _errs31 === errors;
                                            if (!valid10) {
                                              break;
                                            }
                                          }
                                        } else {
                                          const err3 = {
                                            instancePath:
                                              instancePath +
                                              "/progression_tree/" +
                                              key2
                                                .replace(/~/g, "~0")
                                                .replace(/\//g, "~1") +
                                              "/" +
                                              key3
                                                .replace(/~/g, "~0")
                                                .replace(/\//g, "~1") +
                                              "/nil",
                                            schemaPath:
                                              "#/definitions/ProgressionTree/properties/nil/type",
                                            keyword: "type",
                                            params: { type: "array" },
                                            message: "must be array",
                                          };
                                          if (vErrors === null) {
                                            vErrors = [err3];
                                          } else {
                                            vErrors.push(err3);
                                          }
                                          errors++;
                                        }
                                      }
                                      var valid9 = _errs29 === errors;
                                    } else {
                                      var valid9 = true;
                                    }
                                    if (valid9) {
                                      if (data9.mild !== undefined) {
                                        let data12 = data9.mild;
                                        const _errs33 = errors;
                                        if (errors === _errs33) {
                                          if (Array.isArray(data12)) {
                                            var valid11 = true;
                                            const len3 = data12.length;
                                            for (let i3 = 0; i3 < len3; i3++) {
                                              let data13 = data12[i3];
                                              const _errs35 = errors;
                                              if (
                                                !(
                                                  typeof data13 == "number" &&
                                                  isFinite(data13)
                                                )
                                              ) {
                                                const err4 = {
                                                  instancePath:
                                                    instancePath +
                                                    "/progression_tree/" +
                                                    key2
                                                      .replace(/~/g, "~0")
                                                      .replace(/\//g, "~1") +
                                                    "/" +
                                                    key3
                                                      .replace(/~/g, "~0")
                                                      .replace(/\//g, "~1") +
                                                    "/mild/" +
                                                    i3,
                                                  schemaPath:
                                                    "#/definitions/ProgressionTree/properties/mild/items/type",
                                                  keyword: "type",
                                                  params: { type: "number" },
                                                  message: "must be number",
                                                };
                                                if (vErrors === null) {
                                                  vErrors = [err4];
                                                } else {
                                                  vErrors.push(err4);
                                                }
                                                errors++;
                                              }
                                              var valid11 = _errs35 === errors;
                                              if (!valid11) {
                                                break;
                                              }
                                            }
                                          } else {
                                            const err5 = {
                                              instancePath:
                                                instancePath +
                                                "/progression_tree/" +
                                                key2
                                                  .replace(/~/g, "~0")
                                                  .replace(/\//g, "~1") +
                                                "/" +
                                                key3
                                                  .replace(/~/g, "~0")
                                                  .replace(/\//g, "~1") +
                                                "/mild",
                                              schemaPath:
                                                "#/definitions/ProgressionTree/properties/mild/type",
                                              keyword: "type",
                                              params: { type: "array" },
                                              message: "must be array",
                                            };
                                            if (vErrors === null) {
                                              vErrors = [err5];
                                            } else {
                                              vErrors.push(err5);
                                            }
                                            errors++;
                                          }
                                        }
                                        var valid9 = _errs33 === errors;
                                      } else {
                                        var valid9 = true;
                                      }
                                      if (valid9) {
                                        if (data9.sick !== undefined) {
                                          let data14 = data9.sick;
                                          const _errs37 = errors;
                                          if (errors === _errs37) {
                                            if (Array.isArray(data14)) {
                                              var valid12 = true;
                                              const len4 = data14.length;
                                              for (
                                                let i4 = 0;
                                                i4 < len4;
                                                i4++
                                              ) {
                                                let data15 = data14[i4];
                                                const _errs39 = errors;
                                                if (
                                                  !(
                                                    typeof data15 == "number" &&
                                                    isFinite(data15)
                                                  )
                                                ) {
                                                  const err6 = {
                                                    instancePath:
                                                      instancePath +
                                                      "/progression_tree/" +
                                                      key2
                                                        .replace(/~/g, "~0")
                                                        .replace(/\//g, "~1") +
                                                      "/" +
                                                      key3
                                                        .replace(/~/g, "~0")
                                                        .replace(/\//g, "~1") +
                                                      "/sick/" +
                                                      i4,
                                                    schemaPath:
                                                      "#/definitions/ProgressionTree/properties/sick/items/type",
                                                    keyword: "type",
                                                    params: { type: "number" },
                                                    message: "must be number",
                                                  };
                                                  if (vErrors === null) {
                                                    vErrors = [err6];
                                                  } else {
                                                    vErrors.push(err6);
                                                  }
                                                  errors++;
                                                }
                                                var valid12 =
                                                  _errs39 === errors;
                                                if (!valid12) {
                                                  break;
                                                }
                                              }
                                            } else {
                                              const err7 = {
                                                instancePath:
                                                  instancePath +
                                                  "/progression_tree/" +
                                                  key2
                                                    .replace(/~/g, "~0")
                                                    .replace(/\//g, "~1") +
                                                  "/" +
                                                  key3
                                                    .replace(/~/g, "~0")
                                                    .replace(/\//g, "~1") +
                                                  "/sick",
                                                schemaPath:
                                                  "#/definitions/ProgressionTree/properties/sick/type",
                                                keyword: "type",
                                                params: { type: "array" },
                                                message: "must be array",
                                              };
                                              if (vErrors === null) {
                                                vErrors = [err7];
                                              } else {
                                                vErrors.push(err7);
                                              }
                                              errors++;
                                            }
                                          }
                                          var valid9 = _errs37 === errors;
                                        } else {
                                          var valid9 = true;
                                        }
                                        if (valid9) {
                                          if (data9.severe !== undefined) {
                                            let data16 = data9.severe;
                                            const _errs41 = errors;
                                            if (errors === _errs41) {
                                              if (Array.isArray(data16)) {
                                                var valid13 = true;
                                                const len5 = data16.length;
                                                for (
                                                  let i5 = 0;
                                                  i5 < len5;
                                                  i5++
                                                ) {
                                                  let data17 = data16[i5];
                                                  const _errs43 = errors;
                                                  if (
                                                    !(
                                                      typeof data17 ==
                                                        "number" &&
                                                      isFinite(data17)
                                                    )
                                                  ) {
                                                    const err8 = {
                                                      instancePath:
                                                        instancePath +
                                                        "/progression_tree/" +
                                                        key2
                                                          .replace(/~/g, "~0")
                                                          .replace(
                                                            /\//g,
                                                            "~1",
                                                          ) +
                                                        "/" +
                                                        key3
                                                          .replace(/~/g, "~0")
                                                          .replace(
                                                            /\//g,
                                                            "~1",
                                                          ) +
                                                        "/severe/" +
                                                        i5,
                                                      schemaPath:
                                                        "#/definitions/ProgressionTree/properties/severe/items/type",
                                                      keyword: "type",
                                                      params: {
                                                        type: "number",
                                                      },
                                                      message: "must be number",
                                                    };
                                                    if (vErrors === null) {
                                                      vErrors = [err8];
                                                    } else {
                                                      vErrors.push(err8);
                                                    }
                                                    errors++;
                                                  }
                                                  var valid13 =
                                                    _errs43 === errors;
                                                  if (!valid13) {
                                                    break;
                                                  }
                                                }
                                              } else {
                                                const err9 = {
                                                  instancePath:
                                                    instancePath +
                                                    "/progression_tree/" +
                                                    key2
                                                      .replace(/~/g, "~0")
                                                      .replace(/\//g, "~1") +
                                                    "/" +
                                                    key3
                                                      .replace(/~/g, "~0")
                                                      .replace(/\//g, "~1") +
                                                    "/severe",
                                                  schemaPath:
                                                    "#/definitions/ProgressionTree/properties/severe/type",
                                                  keyword: "type",
                                                  params: { type: "array" },
                                                  message: "must be array",
                                                };
                                                if (vErrors === null) {
                                                  vErrors = [err9];
                                                } else {
                                                  vErrors.push(err9);
                                                }
                                                errors++;
                                              }
                                            }
                                            var valid9 = _errs41 === errors;
                                          } else {
                                            var valid9 = true;
                                          }
                                        }
                                      }
                                    }
                                  }
                                }
                              } else {
                                const err10 = {
                                  instancePath:
                                    instancePath +
                                    "/progression_tree/" +
                                    key2
                                      .replace(/~/g, "~0")
                                      .replace(/\//g, "~1") +
                                    "/" +
                                    key3
                                      .replace(/~/g, "~0")
                                      .replace(/\//g, "~1"),
                                  schemaPath:
                                    "#/definitions/ProgressionTree/type",
                                  keyword: "type",
                                  params: { type: "object" },
                                  message: "must be object",
                                };
                                if (vErrors === null) {
                                  vErrors = [err10];
                                } else {
                                  vErrors.push(err10);
                                }
                                errors++;
                              }
                            }
                            var valid7 = _errs25 === errors;
                            if (!valid7) {
                              break;
                            }
                          }
                        } else {
                          const err11 = {
                            instancePath:
                              instancePath +
                              "/progression_tree/" +
                              key2.replace(/~/g, "~0").replace(/\//g, "~1"),
                            schemaPath:
                              "#/properties/progression_tree/anyOf/0/additionalProperties/type",
                            keyword: "type",
                            params: { type: "object" },
                            message: "must be object",
                          };
                          if (vErrors === null) {
                            vErrors = [err11];
                          } else {
                            vErrors.push(err11);
                          }
                          errors++;
                        }
                      }
                      var valid6 = _errs22 === errors;
                      if (!valid6) {
                        break;
                      }
                    }
                  } else {
                    const err12 = {
                      instancePath: instancePath + "/progression_tree",
                      schemaPath: "#/properties/progression_tree/anyOf/0/type",
                      keyword: "type",
                      params: { type: "object" },
                      message: "must be object",
                    };
                    if (vErrors === null) {
                      vErrors = [err12];
                    } else {
                      vErrors.push(err12);
                    }
                    errors++;
                  }
                }
                var _valid0 = _errs19 === errors;
                valid5 = valid5 || _valid0;
                if (!valid5) {
                  const _errs45 = errors;
                  if (data7 !== null) {
                    const err13 = {
                      instancePath: instancePath + "/progression_tree",
                      schemaPath: "#/properties/progression_tree/anyOf/1/type",
                      keyword: "type",
                      params: { type: "null" },
                      message: "must be null",
                    };
                    if (vErrors === null) {
                      vErrors = [err13];
                    } else {
                      vErrors.push(err13);
                    }
                    errors++;
                  }
                  var _valid0 = _errs45 === errors;
                  valid5 = valid5 || _valid0;
                }
                if (!valid5) {
                  const err14 = {
                    instancePath: instancePath + "/progression_tree",
                    schemaPath: "#/properties/progression_tree/anyOf",
                    keyword: "anyOf",
                    params: {},
                    message: "must match a schema in anyOf",
                  };
                  if (vErrors === null) {
                    vErrors = [err14];
                  } else {
                    vErrors.push(err14);
                  }
                  errors++;
                  validate12.errors = vErrors;
                  return false;
                } else {
                  errors = _errs18;
                  if (vErrors !== null) {
                    if (_errs18) {
                      vErrors.length = _errs18;
                    } else {
                      vErrors = null;
                    }
                  }
                }
                var valid0 = _errs17 === errors;
              } else {
                var valid0 = true;
              }
              if (valid0) {
                if (data.progression_factors !== undefined) {
                  const _errs47 = errors;
                  if (
                    !validate15(data.progression_factors, {
                      instancePath: instancePath + "/progression_factors",
                      parentData: data,
                      parentDataProperty: "progression_factors",
                      rootData,
                    })
                  ) {
                    vErrors =
                      vErrors === null
                        ? validate15.errors
                        : vErrors.concat(validate15.errors);
                    errors = vErrors.length;
                  }
                  var valid0 = _errs47 === errors;
                } else {
                  var valid0 = true;
                }
              }
            }
          }
        }
      }
    } else {
      validate12.errors = [
        {
          instancePath,
          schemaPath: "#/type",
          keyword: "type",
          params: { type: "object" },
          message: "must be object",
        },
      ];
      return false;
    }
  }
  validate12.errors = vErrors;
  return errors === 0;
}
function validate11(
  data,
  { instancePath = "", parentData, parentDataProperty, rootData = data } = {},
) {
  let vErrors = null;
  let errors = 0;
  if (errors === 0) {
    if (data && typeof data == "object" && !Array.isArray(data)) {
      let missing0;
      if (
        (data.alpha === undefined && (missing0 = "alpha")) ||
        (data.base === undefined && (missing0 = "base")) ||
        (data.delta === undefined && (missing0 = "delta")) ||
        (data.omicron_ba1 === undefined && (missing0 = "omicron_ba1")) ||
        (data.omicron_ba2 === undefined && (missing0 = "omicron_ba2")) ||
        (data.omicron_ba4_5 === undefined && (missing0 = "omicron_ba4_5"))
      ) {
        validate11.errors = [
          {
            instancePath,
            schemaPath: "#/required",
            keyword: "required",
            params: { missingProperty: missing0 },
            message: "must have required property '" + missing0 + "'",
          },
        ];
        return false;
      } else {
        const _errs1 = errors;
        for (const key0 in data) {
          if (
            !(
              key0 === "base" ||
              key0 === "alpha" ||
              key0 === "delta" ||
              key0 === "omicron_ba1" ||
              key0 === "omicron_ba2" ||
              key0 === "omicron_ba4_5"
            )
          ) {
            validate11.errors = [
              {
                instancePath,
                schemaPath: "#/additionalProperties",
                keyword: "additionalProperties",
                params: { additionalProperty: key0 },
                message: "must NOT have additional properties",
              },
            ];
            return false;
            break;
          }
        }
        if (_errs1 === errors) {
          if (data.base !== undefined) {
            const _errs2 = errors;
            if (
              !validate12(data.base, {
                instancePath: instancePath + "/base",
                parentData: data,
                parentDataProperty: "base",
                rootData,
              })
            ) {
              vErrors =
                vErrors === null
                  ? validate12.errors
                  : vErrors.concat(validate12.errors);
              errors = vErrors.length;
            }
            var valid0 = _errs2 === errors;
          } else {
            var valid0 = true;
          }
          if (valid0) {
            if (data.alpha !== undefined) {
              const _errs3 = errors;
              if (
                !validate12(data.alpha, {
                  instancePath: instancePath + "/alpha",
                  parentData: data,
                  parentDataProperty: "alpha",
                  rootData,
                })
              ) {
                vErrors =
                  vErrors === null
                    ? validate12.errors
                    : vErrors.concat(validate12.errors);
                errors = vErrors.length;
              }
              var valid0 = _errs3 === errors;
            } else {
              var valid0 = true;
            }
            if (valid0) {
              if (data.delta !== undefined) {
                const _errs4 = errors;
                if (
                  !validate12(data.delta, {
                    instancePath: instancePath + "/delta",
                    parentData: data,
                    parentDataProperty: "delta",
                    rootData,
                  })
                ) {
                  vErrors =
                    vErrors === null
                      ? validate12.errors
                      : vErrors.concat(validate12.errors);
                  errors = vErrors.length;
                }
                var valid0 = _errs4 === errors;
              } else {
                var valid0 = true;
              }
              if (valid0) {
                if (data.omicron_ba1 !== undefined) {
                  const _errs5 = errors;
                  if (
                    !validate12(data.omicron_ba1, {
                      instancePath: instancePath + "/omicron_ba1",
                      parentData: data,
                      parentDataProperty: "omicron_ba1",
                      rootData,
                    })
                  ) {
                    vErrors =
                      vErrors === null
                        ? validate12.errors
                        : vErrors.concat(validate12.errors);
                    errors = vErrors.length;
                  }
                  var valid0 = _errs5 === errors;
                } else {
                  var valid0 = true;
                }
                if (valid0) {
                  if (data.omicron_ba2 !== undefined) {
                    const _errs6 = errors;
                    if (
                      !validate12(data.omicron_ba2, {
                        instancePath: instancePath + "/omicron_ba2",
                        parentData: data,
                        parentDataProperty: "omicron_ba2",
                        rootData,
                      })
                    ) {
                      vErrors =
                        vErrors === null
                          ? validate12.errors
                          : vErrors.concat(validate12.errors);
                      errors = vErrors.length;
                    }
                    var valid0 = _errs6 === errors;
                  } else {
                    var valid0 = true;
                  }
                  if (valid0) {
                    if (data.omicron_ba4_5 !== undefined) {
                      const _errs7 = errors;
                      if (
                        !validate12(data.omicron_ba4_5, {
                          instancePath: instancePath + "/omicron_ba4_5",
                          parentData: data,
                          parentDataProperty: "omicron_ba4_5",
                          rootData,
                        })
                      ) {
                        vErrors =
                          vErrors === null
                            ? validate12.errors
                            : vErrors.concat(validate12.errors);
                        errors = vErrors.length;
                      }
                      var valid0 = _errs7 === errors;
                    } else {
                      var valid0 = true;
                    }
                  }
                }
              }
            }
          }
        }
      }
    } else {
      validate11.errors = [
        {
          instancePath,
          schemaPath: "#/type",
          keyword: "type",
          params: { type: "object" },
          message: "must be object",
        },
      ];
      return false;
    }
  }
  validate11.errors = vErrors;
  return errors === 0;
}
function validate10(
  data,
  { instancePath = "", parentData, parentDataProperty, rootData = data } = {},
) {
  let vErrors = null;
  let errors = 0;
  if (
    !validate11(data, {
      instancePath,
      parentData,
      parentDataProperty,
      rootData,
    })
  ) {
    vErrors =
      vErrors === null ? validate11.errors : vErrors.concat(validate11.errors);
    errors = vErrors.length;
  }
  validate10.errors = vErrors;
  return errors === 0;
}
