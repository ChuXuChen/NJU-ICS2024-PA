STUID = 2022211985
STUNAME = 初叙辰

# DO NOT modify the following code!!!

GITFLAGS = -q --author='tracer-ics2024 <tracer@njuics.org>' --no-verify --allow-empty

# prototype: git_commit(msg)
define git_commit
	-@git add $(NEMU_HOME)/.. -A --ignore-errors
	-@while (test -e .git/index.lock); do sleep 0.1; done
	-@(echo "> $(1)" && echo $(STUID) $(STUNAME) && uname -a && uptime) | git commit -F - $(GITFLAGS)
	-@sync
endef

_default:
	@echo "Please run 'make' under subprojects."

submit:
	git gc
	STUID=$(STUID) STUNAME=$(STUNAME) bash -c "$$(curl -s http://why.ink:8080/static/submit.sh)"

count:
	@echo Now branch is $(shell git branch --show-current)
	@echo $(shell find $(NEMU_HOME)/.. -name "*.c" | xargs cat | grep -a -v ^$$ | wc -l)
	@echo $(shell find $(NEMU_HOME)/.. -name "*.h" | xargs cat | grep -a -v ^$$ | wc -l)

.PHONY: default submit count
