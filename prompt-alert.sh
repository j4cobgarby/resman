function _resman_prompt_command {
    if ! resman; then
        printf "\033[0;31m(${_RESMAN_WORD})\033[0m ";
    fi
}

PROMPT_COMMAND=_resman_prompt_command
_RESMAN_WORD="server in use!"
