Docker CICD images
------------------

To decouple the building project from vendor-locked CI/CD systems, you must
containerize the environment using Docker.

1. Dockerfile.android - Android building environment
2. Dockerfile.linux - Linux building environment

```bash
$ docker build -f Dockerfile.linux -t linux-env .
$ docker build -f Dockerfile.android -t android-env .
```

Before execution modify the CI/CD pipeline scripts to specify the correct
build image.
