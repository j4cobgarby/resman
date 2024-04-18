_RESMAN_WORD="server in use..."
PS1='$(if resman; then echo; else printf "\[\033[0;31m\](${_RESMAN_WORD})\[\033[0m\] "; fi)'"${PS1}"
