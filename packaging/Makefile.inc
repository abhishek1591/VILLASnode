TAROPTS = --exclude-ignore-recursive=.distignore --transform='s|^\.|villas-node-$(VERSION_NUM)|' --show-transformed-names

TAR_VILLAS = $(BUILDDIR)/packaging/villas-node-$(VERSION_NUM)-1.$(GIT_BRANCH)_$(subst -,_,$(VARIANT)).$$(date +%Y%m%d)git$(GIT_REV).tar.gz

DEPLOY_HOST = root@villas.0l.de
DEPLOY_PATH = /var/www/villas

packaging: rpm dist
	
deploy: deploy-dist deploy-rpm
	
# Source tarballs
dist: $(TAR_VILLAS)

$(TAR_VILLAS): | $$(dir $$@)
	tar $(TAROPTS) -C $(SRCDIR) -czf $@ .

deploy-dist: $(TAR_VILLAS)
	rsync $(TAR_VILLAS) $(DEPLOY_HOST):$(DEPLOY_PATH)/node/src
	
deploy-rpm:
	rsync -a --progress $(RPMDIR)/RPMS/ $(DEPLOY_HOST):$(DEPLOY_PATH)/packages/
	ssh $(DEPLOY_HOST) createrepo $(DEPLOY_PATH)/packages

# Docker targets
run-docker-dev:
	docker run -it -p 80:80 -p 443:443 -p 1234:1234 --privileged --security-opt seccomp:unconfined -v $(PWD):/villas villas-node-dev

docker-dev:
	docker build -f Dockerfile.dev -t villas-node-dev $(SRCDIR)
	
docker:
	docker build -f Dockerfile -t villas-node $(SRCDIR)
	
clean-packaging:
	rm -f $(BUILDDIR)/packaging/villas-node-$(VERSION_NUM).tar.gz

install-packaging:

.PHONY: packaging install-packaging clean-packaging deploy deploy-dist deploy-rpm dist docker docker-dev run-docker-dev $(TAR_VILLAS)

-include packaging/rpm/Makefile.inc