[[ $- != *i* ]] && return
alias ls='ls --color=auto'
PS1='\[\e[32m\]\w\[\e[0m\] $ '

alias mango='dbus-run-session mango'
alias c='clear'
alias fuck='doas'
alias sudo='doas'
alias p9='vfetch -d plan9'
alias minecraft='java -jar $HOME/Apps/SKlauncher-3.2.18.jar'
alias ll='ls -la'
alias i='doas xbps-install -S'
alias q='doas xbps-query -Rs'
alias r='doas xbps-remove'
alias fp='fuzzypkg'

set -o vi
export EDITOR=vim
