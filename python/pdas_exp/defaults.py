import itertools

nvars = {
    "2d_swe": 3,
    "2d_euler": 4,
}

problem_options = {
    "2d_swe": ["SlipWall"],
    "2d_euler": ["NormalShock", "Riemann"],
}

phys_params_default = {
    "2d_swe": {
        "coriolis": -3.0,
        "gravity": 9.8,
    },
    "2d_euler": {
        "gamma": 1.4,
    },
}

ic_params_default = {
    "2d_swe": {
        "SlipWall": {
            "icFlag1": {
                "pulseMagnitude": 0.125,
                "pulseX": 1.0,
                "pulseY": 1.0,
            },
            "icFlag2": {
                "pulseMagnitude1": 0.1,
                "pulseX1": -2.0,
                "pulseY1": -2.0,
                "pulseMagnitude1": 0.125,
                "pulseX1": 2.0,
                "pulseY1": 2.0,
            },
        }
    },
    "2d_euler": {
        "Riemann": {
            "icFlag1": {
                "riemannTopRightPressure": 0.4,
            },
            "icFlag2": {
                "riemannTopRightPressure": 1.5,
                "riemannTopRightXVel": 0.0,
                "riemannTopRightYVel": 0.0,
                "riemannTopRightDensity": 1.5,
                "riemannBotLeftPressure": 0.029,
            },
        },
    },
}

def check_params(phys_params_user, ic_params_user, equations, problem, icFlag):
    assert equations in problem_options # valid equation
    assert problem in problem_options[equations] # valid problem
    # valid physical parameters
    for param in phys_params_user:
        assert param in phys_params_default[equations]
    assert f"icFlag{icFlag}" in ic_params_default[equations][problem] # valid icParam
    # valid IC parameters
    for param in ic_params_user:
        assert param in ic_params_default[equations][problem][f"icFlag{icFlag}"]

def get_params_combo(phys_params_user, ic_params_user, equations, problem, icFlag):
    params_names_list = []
    params_vals_list = []
    for param in phys_params_default[equations]:
        params_names_list.append(param)
        if param in phys_params_user:
            params_vals_list.append(phys_params_user[param])
        else:
            params_vals_list.append([phys_params_default[equations][param]])
    for param in ic_params_default[equations][problem][f"icFlag{icFlag}"]:
        params_names_list.append(param)
        if param in ic_params_user:
            params_vals_list.append(ic_params_user[param])
        else:
            params_vals_list.append([ic_params_default[equations][problem][f"icFlag{icFlag}"][param]])

    params_combo = list(itertools.product(*params_vals_list))
    return params_names_list, params_combo