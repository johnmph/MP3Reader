#ifndef MP3_HELPER_S_HPP
#define MP3_HELPER_S_HPP


template <typename TContainer, class TFunction>
constexpr TContainer transformContainer(TContainer const &input, TFunction &&function) {    //TODO: ATTENTION: si vector, taille pas initialisée
    TContainer output = {};

    for (std::size_t index = 0; index < input.size(); ++index) {
        output[index] = function(input[index]);
    }

    return output;
}


#endif /* MP3_HELPER_S_HPP */
