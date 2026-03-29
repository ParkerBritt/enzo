struct ConnectionSerializable
{
    unsigned int inputNodeIndex = 0;
    unsigned int inputSocketIndex = 0;
    unsigned int outputNodeIndex = 0;
    unsigned int outputSocketIndex = 0;

    template <class Archive>
    void serialize( Archive & ar )
    {
        ar( CEREAL_NVP(inputNodeIndex), CEREAL_NVP(inputSocketIndex),
            CEREAL_NVP(outputNodeIndex), CEREAL_NVP(outputSocketIndex) );
    }
};
