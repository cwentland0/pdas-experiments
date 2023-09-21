import os

def check_meshdir(dirpath):
    assert os.path.isfile(os.path.join(dirpath, "connectivity.dat"))
    assert os.path.isfile(os.path.join(dirpath, "coordinates.dat"))
    assert os.path.isfile(os.path.join(dirpath, "info.dat"))


def mkdir(dirpath):
    if not os.path.isdir(dirpath):
        os.mkdir(dirpath)


def catchlist(var, dtype, count):
    if not isinstance(var, list):
        assert isinstance(var, dtype)
        var = [var] * count
    assert len(var) == count
    return var