
{
  loadPlatform: platform [

    {
      HashNode = platform HashNode.

      buckets := platform Array newSize: 8.
      capacityMask := 8-1.
      size := 0.

      at: key [
        hash := key hash.
        index := hash & capacityMask.

        node := buckets at: index.
        [ node isNil ] whileFalse: [
          ((node hash equals: hash) and: (node key equals: key))
            ifTrue: [
              ^ node value
            ].

          index: (index + 1) & capacityMask.
          node: (buckets at: index)
        ].

        node: (HashNode clone key: key, value: value, hash: hash)
      ].

      at: key put: value [
        hash := key hash.
        index := hash & capacityMask.

        node := buckets at: index.
        [ node isNil ] whileFalse: [
          ((node hash equals: hash) and: (node key equals: key))
            ifTrue: [
              node value: value.
              ^ self
            ].

          index: (index + 1) & capacityMask.
          node: (buckets at: index)
        ].

        node: (HashNode clone key: key, value: value, hash: hash).
        buckets at: index put: node.
      ].


    }
  ]
}